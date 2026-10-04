#include "sourceimage.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <iterator>
#include <optional>
#include <regex>
#include <system_error>

#include "byteorder.h"
#include "imageerror.h"

namespace {

constexpr std::size_t kSectorRaw = 2352;
constexpr std::size_t kMode1Offset = 16;
constexpr std::size_t kMode2Offset = 24;
constexpr std::uint32_t kFramesPerMinute = 4500;
constexpr std::uint32_t kFramesPerSecond = 75;

// The offsets below are within the primary volume descriptor.
constexpr std::size_t kPvdVolumeSectors = 80;
constexpr std::size_t kPvdRootLba = 158;
constexpr std::size_t kPvdRootSize = 166;

// The constants below describe a directory record.
constexpr std::size_t kRecordLba = 2;
constexpr std::size_t kRecordSize = 10;
constexpr std::size_t kRecordFlags = 25;
constexpr std::size_t kRecordNameLength = 32;
constexpr std::size_t kRecordName = 33;
constexpr std::uint8_t kFlagDirectory = 0x02;
constexpr std::uint8_t kFlagMultiExtent = 0x80;

// Splits text at the line breaks of Python's `str.splitlines` that occur in a cue sheet.
std::vector<std::string> splitLines(const std::string &text) {
    std::vector<std::string> lines;
    std::string line;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto character = text[i];
        if (character == '\r' || character == '\n') {
            if (character == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            lines.push_back(std::move(line));
            line.clear();
        } else {
            line += character;
        }
    }
    if (!line.empty()) {
        lines.push_back(std::move(line));
    }
    return lines;
}

// The regular expressions match only digits. The conversion fails only on overflow.
std::uint32_t toNumber(const std::ssub_match &match) {
    std::uint32_t value = 0;
    std::from_chars(&*match.first, &*match.first + match.length(), value);
    return value;
}

} // namespace

SourceImage::SourceImage(std::ifstream handle, Geometry geometry)
    : handle_(std::move(handle)), geometry_(geometry) {
}

std::expected<std::unique_ptr<SourceImage>, Error>
SourceImage::open(const std::filesystem::path &path) {
    auto layout = detectGeometry(path);
    if (!layout) {
        return std::unexpected(std::move(layout.error()));
    }
    std::ifstream handle(layout->first, std::ios::binary);
    if (!handle) {
        return ioError(std::format("Cannot read {}.", layout->first.string()));
    }
    std::unique_ptr<SourceImage> image(new SourceImage(std::move(handle), layout->second));
    Sector descriptor;
    if (auto read = image->readSector(kPvdSector, descriptor); !read) {
        return std::unexpected(std::move(read.error()));
    }
    if (descriptor[0] != 1 ||
        !std::equal(kPvdMagic.begin(), kPvdMagic.end(), descriptor.begin() + 1)) {
        return discImageError(std::format("{} has no primary volume descriptor.", path.string()));
    }
    image->volumeSectors_ = readLittle32(descriptor, kPvdVolumeSectors);
    return image;
}

std::expected<LocatedFile, Error> SourceImage::find(const std::string &name) {
    Sector descriptor;
    if (auto read = readSector(kPvdSector, descriptor); !read) {
        return std::unexpected(std::move(read.error()));
    }
    std::vector<std::pair<std::uint32_t, std::uint32_t>> stack{
        {readLittle32(descriptor, kPvdRootLba), readLittle32(descriptor, kPvdRootSize)}};
    while (!stack.empty()) {
        const auto [parentLba, parentSize] = stack.back();
        stack.pop_back();
        const auto listing = records(parentLba, parentSize);
        if (!listing) {
            return std::unexpected(listing.error());
        }
        for (const auto &record : *listing) {
            if (record.name == std::string_view("\0", 1) ||
                record.name == std::string_view("\1", 1)) {
                continue;
            }
            if (record.isDirectory) {
                stack.emplace_back(record.lba, record.size);
            } else if (upperAscii(record.name.substr(0, record.name.find(';'))) == name) {
                if (record.multiExtent) {
                    return discImageError(std::format("{} spans several extents.", name));
                }
                return LocatedFile{record.lba, parentLba, parentSize, record.size};
            }
        }
    }
    return discImageError(std::format("{} is missing from the image.", name));
}

std::expected<void, Error> SourceImage::readSector(std::uint32_t lba,
                                                   std::span<std::uint8_t, kSectorData> sector) {
    if (readSectors(lba, sector).value_or(0) != 1) {
        return pastEnd(lba);
    }
    return {};
}

std::expected<std::size_t, Error> SourceImage::readSectors(std::uint32_t lba,
                                                           std::span<std::uint8_t> sectors) {
    const auto count = sectors.size() / kSectorData;
    const auto start = static_cast<std::streamoff>(
        (static_cast<std::uint64_t>(geometry_.trackStart) + lba) * geometry_.sectorSize);
    handle_.clear();
    handle_.seekg(start);
    if (geometry_.sectorSize == kSectorData) {
        // The streams read char.
        handle_.read(reinterpret_cast<char *>(sectors.data()),
                     static_cast<std::streamsize>(count * kSectorData));
        return static_cast<std::size_t>(handle_.gcount()) / kSectorData;
    }
    rawBuffer_.resize(count * geometry_.sectorSize);
    handle_.read(rawBuffer_.data(), static_cast<std::streamsize>(rawBuffer_.size()));
    const auto got = static_cast<std::size_t>(handle_.gcount());
    std::size_t complete = 0;
    for (; complete < count; ++complete) {
        const auto offset = complete * geometry_.sectorSize + geometry_.dataOffset;
        if (offset + kSectorData > got) {
            break;
        }
        std::copy_n(rawBuffer_.begin() + static_cast<std::ptrdiff_t>(offset),
                    kSectorData,
                    sectors.begin() + static_cast<std::ptrdiff_t>(complete * kSectorData));
    }
    return complete;
}

std::uint32_t SourceImage::volumeSectors() const {
    return volumeSectors_;
}

std::expected<SourceImage::Layout, Error>
SourceImage::detectGeometry(const std::filesystem::path &path) {
    const auto suffix = lowerAscii(path.extension().string());
    if (suffix == ".cue") {
        return parseCue(path);
    }
    if (suffix == ".bin") {
        return Layout{path, {kMode2Offset, kSectorRaw, 0}};
    }
    if (suffix == ".iso") {
        return Layout{path, {0, kSectorData, 0}};
    }
    return discImageError(
        std::format("The disc image suffix {} is not supported.", path.extension().string()));
}

std::expected<SourceImage::Layout, Error> SourceImage::parseCue(const std::filesystem::path &path) {
    static const std::regex kFile(R"re(FILE\s+"([^"]+)")re", std::regex::icase);
    static const std::regex kTrack(R"(TRACK\s+(\d+)\s+(\S+))", std::regex::icase);
    static const std::regex kIndex(R"(INDEX\s+01\s+(\d+):(\d+):(\d+))", std::regex::icase);
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return ioError(std::format("Cannot read {}.", path.string()));
    }
    const std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    std::optional<std::string> binName;
    std::optional<std::string> trackMode;
    std::uint32_t trackStart = 0;
    for (const auto &line : splitLines(text)) {
        std::smatch found;
        if (std::regex_search(line, found, kFile)) {
            binName = found[1].str();
        } else if (std::regex_search(line, found, kTrack)) {
            if (toNumber(found[1]) == 1) {
                trackMode = found[2].str();
            }
        } else if (std::regex_search(line, found, kIndex) && trackMode) {
            trackStart = toNumber(found[1]) * kFramesPerMinute +
                         toNumber(found[2]) * kFramesPerSecond + toNumber(found[3]);
            break;
        }
    }
    if (!binName || !trackMode) {
        return discImageError(
            std::format("The cue sheet {} lacks a binary track one.", path.string()));
    }
    auto candidate = resolveBinPath(path.parent_path(), *binName);
    if (!candidate) {
        return std::unexpected(std::move(candidate.error()));
    }
    const auto mode = upperAscii(*trackMode);
    std::size_t offset = 0;
    if (mode == "MODE1/2352") {
        offset = kMode1Offset;
    } else if (mode == "MODE2/2352") {
        offset = kMode2Offset;
    } else {
        return discImageError(std::format("The track mode {} is not supported.", *trackMode));
    }
    return Layout{std::move(*candidate), {offset, kSectorRaw, trackStart}};
}

std::expected<std::filesystem::path, Error>
SourceImage::resolveBinPath(const std::filesystem::path &directory, const std::string &binName) {
    const auto searched = directory.empty() ? std::filesystem::path(".") : directory;
    auto candidate = searched / binName;
    std::error_code error;
    if (std::filesystem::exists(candidate, error)) {
        return candidate;
    }
    const auto lowered = lowerAscii(binName);
    for (std::filesystem::directory_iterator it(searched, error), end; !error && it != end;
         it.increment(error)) {
        if (it->is_regular_file(error) && lowerAscii(it->path().filename().string()) == lowered) {
            return it->path();
        }
    }
    return discImageError(
        std::format("Binary {} is missing beside {}.", binName, searched.string()));
}

std::expected<std::vector<SourceImage::Record>, Error> SourceImage::records(std::uint32_t lba,
                                                                            std::uint32_t size) {
    std::vector<std::uint8_t> extent(static_cast<std::size_t>(sectorsFor(size)) * kSectorData);
    for (std::uint32_t i = 0; i < sectorsFor(size); ++i) {
        if (auto read = readSector(lba + i,
                                   std::span(extent).subspan(i * kSectorData).first<kSectorData>());
            !read) {
            return std::unexpected(std::move(read.error()));
        }
    }
    std::vector<Record> found;
    std::size_t offset = 0;
    while (offset < extent.size()) {
        const auto length = extent[offset];
        if (length == 0) {
            offset = (offset / kSectorData + 1) * kSectorData;
            continue;
        }
        const auto record = std::span(extent).subspan(offset);
        if (record.size() <= kRecordNameLength ||
            kRecordName + record[kRecordNameLength] > record.size()) {
            return discImageError(std::format("Directory record at sector {} is truncated.", lba));
        }
        const auto flags = record[kRecordFlags];
        const auto name = record.subspan(kRecordName, record[kRecordNameLength]);
        found.push_back({(flags & kFlagDirectory) != 0,
                         readLittle32(record, kRecordLba),
                         (flags & kFlagMultiExtent) != 0,
                         std::string(name.begin(), name.end()),
                         readLittle32(record, kRecordSize)});
        offset += length;
    }
    return found;
}
