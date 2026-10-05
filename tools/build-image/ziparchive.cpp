#include "ziparchive.h"

#include <algorithm>
#include <format>

#include <zlib.h>

#include "byteorder.h"
#include "imageerror.h"

namespace Tools::BuildImage {

namespace {

constexpr std::uint32_t kEndSignature = 0x06054B50;
constexpr std::uint32_t kCentralSignature = 0x02014B50;
constexpr std::uint32_t kLocalSignature = 0x04034B50;
constexpr std::size_t kEndLength = 22;
constexpr std::size_t kMaximumCommentLength = 0xFFFF;
constexpr std::size_t kCentralLength = 46;
constexpr std::size_t kLocalLength = 30;
constexpr std::uint16_t kMethodStored = 0;
constexpr std::uint16_t kMethodDeflated = 8;
// zlib reads a raw deflate stream, without a zlib header, when the window bits are negative.
constexpr int kRawDeflateWindowBits = -MAX_WBITS;

// The offsets below are within the end of central directory record.
constexpr std::size_t kEndCount = 10;
constexpr std::size_t kEndDirectoryOffset = 16;

// The offsets below are within a central directory header.
constexpr std::size_t kCentralMethod = 10;
constexpr std::size_t kCentralCrc = 16;
constexpr std::size_t kCentralCompressedSize = 20;
constexpr std::size_t kCentralSize = 24;
constexpr std::size_t kCentralNameLength = 28;
constexpr std::size_t kCentralExtraLength = 30;
constexpr std::size_t kCentralCommentLength = 32;
constexpr std::size_t kCentralLocalOffset = 42;

// The offsets below are within a local file header.
constexpr std::size_t kLocalNameLength = 26;
constexpr std::size_t kLocalExtraLength = 28;

std::unexpected<Error> notZip() {
    return artifactError("Artifact payload is not a zip archive.");
}

} // namespace

ZipArchive::ZipArchive(std::vector<std::uint8_t> data) : data_(std::move(data)) {
}

std::expected<ZipArchive, Error> ZipArchive::open(std::vector<std::uint8_t> data) {
    ZipArchive archive(std::move(data));
    const std::span<const std::uint8_t> bytes = archive.data_;
    if (bytes.size() < kEndLength) {
        return notZip();
    }
    // ponytail: ZIP64 archives are not read. The artifacts are a few megabytes; read the ZIP64
    // end record if an artifact grows past 4 GB or 65535 members.
    // The end record is the last one with its signature, followed by its comment.
    auto end = bytes.size() - kEndLength;
    const auto lowest = end > kMaximumCommentLength ? end - kMaximumCommentLength : 0;
    while (readLittle32(bytes, end) != kEndSignature) {
        if (end == lowest) {
            return notZip();
        }
        --end;
    }
    const auto count = readLittle16(bytes, end + kEndCount);
    std::size_t offset = readLittle32(bytes, end + kEndDirectoryOffset);
    for (std::uint16_t i = 0; i < count; ++i) {
        if (offset + kCentralLength > bytes.size() ||
            readLittle32(bytes, offset) != kCentralSignature) {
            return notZip();
        }
        const auto nameLength = readLittle16(bytes, offset + kCentralNameLength);
        const auto extraLength = readLittle16(bytes, offset + kCentralExtraLength);
        const auto commentLength = readLittle16(bytes, offset + kCentralCommentLength);
        if (offset + kCentralLength + nameLength > bytes.size()) {
            return notZip();
        }
        const auto name = bytes.subspan(offset + kCentralLength, nameLength);
        archive.members_.push_back({std::string(name.begin(), name.end()),
                                    readLittle16(bytes, offset + kCentralMethod),
                                    readLittle32(bytes, offset + kCentralCrc),
                                    readLittle32(bytes, offset + kCentralCompressedSize),
                                    readLittle32(bytes, offset + kCentralSize),
                                    readLittle32(bytes, offset + kCentralLocalOffset)});
        offset += kCentralLength + nameLength + extraLength + commentLength;
    }
    return archive;
}

std::vector<std::string> ZipArchive::names() const {
    std::vector<std::string> names;
    for (const auto &member : members_) {
        names.push_back(member.name);
    }
    return names;
}

std::expected<std::vector<std::uint8_t>, Error> ZipArchive::extract(const std::string &name) const {
    const auto member = std::ranges::find(members_, name, &Member::name);
    if (member == members_.end()) {
        return artifactError(std::format("Member {} is missing from the archive.", name));
    }
    const std::span<const std::uint8_t> bytes = data_;
    const std::size_t local = member->localOffset;
    if (local + kLocalLength > bytes.size() || readLittle32(bytes, local) != kLocalSignature) {
        return notZip();
    }
    const auto start = local + kLocalLength + readLittle16(bytes, local + kLocalNameLength) +
                       readLittle16(bytes, local + kLocalExtraLength);
    if (start + member->compressedSize > bytes.size()) {
        return notZip();
    }
    const auto compressed = bytes.subspan(start, member->compressedSize);
    std::vector<std::uint8_t> contents(member->size);
    if (member->method == kMethodStored) {
        if (member->compressedSize != member->size) {
            return notZip();
        }
        std::ranges::copy(compressed, contents.begin());
    } else if (member->method == kMethodDeflated) {
        z_stream stream{};
        if (inflateInit2(&stream, kRawDeflateWindowBits) != Z_OK) {
            return notZip();
        }
        // zlib does not write through next_in, but its declaration is not const.
        stream.next_in = const_cast<Bytef *>(compressed.data());
        stream.avail_in = static_cast<uInt>(compressed.size());
        stream.next_out = contents.data();
        stream.avail_out = static_cast<uInt>(contents.size());
        const auto status = inflate(&stream, Z_FINISH);
        const auto produced = stream.total_out;
        inflateEnd(&stream);
        if (status != Z_STREAM_END || produced != contents.size()) {
            return notZip();
        }
    } else {
        return artifactError(
            std::format("Member {} uses unsupported compression method {}.", name, member->method));
    }
    if (crc32(0, contents.data(), static_cast<uInt>(contents.size())) != member->crc) {
        return notZip();
    }
    return contents;
}

} // namespace Tools::BuildImage
