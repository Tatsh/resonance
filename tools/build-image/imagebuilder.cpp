#include "imagebuilder.h"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <span>
#include <spdlog/spdlog.h>
#include <string_view>

#include "byteorder.h"
#include "fileio.h"
#include "imageerror.h"
#include "sectorencoder.h"

namespace {

constexpr std::uint32_t kPostgapSectors = 150;
constexpr std::size_t kBatchSectors = 1024;
constexpr std::string_view kElfMagic = "\x7f"
                                       "ELF";
constexpr std::size_t kVerifyLength = 4;
constexpr std::size_t kPvdVolumeSectors = 80;

// The offsets below are within a directory record.
constexpr std::size_t kRecordLba = 2;
constexpr std::size_t kRecordSize = 10;
constexpr std::size_t kRecordNameLength = 32;
constexpr std::size_t kRecordName = 33;

constexpr std::string_view kCueTemplate =
    "FILE \"{}\" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n";

} // namespace

ImageBuilder::ImageBuilder(Volume &source, std::vector<Replacement> replacements)
    : source_(&source), replacements_(std::move(replacements)) {
}

std::expected<ImageBuilder, Error> ImageBuilder::plan(Volume &source,
                                                      std::vector<Replacement> replacements) {
    ImageBuilder builder(source, std::move(replacements));
    if (auto placed = builder.placePayloads(); !placed) {
        return std::unexpected(std::move(placed.error()));
    }
    return builder;
}

std::expected<void, Error> ImageBuilder::placePayloads() {
    constexpr auto kSectorData = Volume::kSectorData;
    const auto sourceSectors = source_->volumeSectors();
    auto appendLba = sourceSectors;
    for (const auto &[target, payload] : replacements_) {
        if (!std::string_view(reinterpret_cast<const char *>(payload.data()), payload.size())
                 .starts_with(kElfMagic)) {
            spdlog::warn("Payload for `{}` lacks an ELF header.", target);
        }
        const auto found = source_->find(target);
        if (!found) {
            return std::unexpected(found.error());
        }
        if (payload.size() <= found->size) {
            for (std::uint32_t index = 0; index < Volume::sectorsFor(found->size); ++index) {
                const auto start = std::size_t{index} * kSectorData;
                const auto end = std::min(start + kSectorData, std::size_t{found->size});
                Volume::Sector chunk{};
                if (end - start < kSectorData) {
                    if (auto read = source_->readSector(found->lba + index, chunk); !read) {
                        return read;
                    }
                }
                for (auto i = start; i < end; ++i) {
                    chunk[i - start] = i < payload.size() ? payload[i] : 0;
                }
                overlays_[found->lba + index] = chunk;
            }
            written_.emplace_back(target, found->lba);
            spdlog::info("Replaced {} in place ({} bytes).", target, payload.size());
            continue;
        }
        const auto sectors = Volume::sectorsFor(payload.size());
        for (std::uint32_t index = 0; index < sectors; ++index) {
            Volume::Sector chunk{};
            const auto slice = std::span(payload).subspan(std::size_t{index} * kSectorData);
            std::ranges::copy(slice.first(std::min(slice.size(), kSectorData)), chunk.begin());
            overlays_[appendLba + index] = chunk;
        }
        for (std::uint32_t index = 0; index < Volume::sectorsFor(found->parentSize); ++index) {
            const auto parentLba = found->parentLba + index;
            auto [parent, inserted] = overlays_.try_emplace(parentLba);
            if (inserted) {
                if (auto read = source_->readSector(parentLba, parent->second); !read) {
                    return read;
                }
            }
            // Like the original tool, every sector of the directory must have the record.
            if (auto patched = patchRecord(parent->second,
                                           found->lba,
                                           target,
                                           appendLba,
                                           static_cast<std::uint32_t>(payload.size()));
                !patched) {
                return patched;
            }
        }
        written_.emplace_back(target, appendLba);
        spdlog::info("Relocated {} to sector {} ({} bytes).", target, appendLba, payload.size());
        appendLba += sectors;
    }
    if (appendLba != sourceSectors) {
        Volume::Sector descriptor;
        if (auto read = source_->readSector(Volume::kPvdSector, descriptor); !read) {
            return read;
        }
        writeLittle(descriptor, kPvdVolumeSectors, appendLba);
        writeBig(descriptor, kPvdVolumeSectors + 4, appendLba);
        overlays_[Volume::kPvdSector] = descriptor;
    }
    volumeSectors_ = appendLba;
    return {};
}

std::expected<std::uint32_t, Error> ImageBuilder::write(const std::filesystem::path &output) {
    constexpr auto kSectorData = Volume::kSectorData;
    constexpr auto kRawSize = SectorEncoder::kRawSize;
    static constexpr Volume::Sector kZeroSector{};
    auto image = output;
    image.replace_extension(".bin");
    std::ofstream file(image, std::ios::binary | std::ios::trunc);
    if (!file) {
        return ioError(std::format("Cannot write {}.", image.string()));
    }
    const auto total = volumeSectors_ + kPostgapSectors;
    const auto readLimit = std::min(source_->volumeSectors(), volumeSectors_);
    std::vector<std::uint8_t> input(kBatchSectors * kSectorData);
    std::vector<std::uint8_t> encoded(kBatchSectors * kRawSize);
    for (std::uint32_t lba = 0; lba < total;) {
        const auto count = std::min<std::size_t>(kBatchSectors, total - lba);
        const auto readable = lba < readLimit ? std::min<std::size_t>(count, readLimit - lba) : 0;
        std::size_t available = 0;
        if (readable != 0) {
            const auto read =
                source_->readSectors(lba, std::span(input).first(readable * kSectorData));
            if (!read) {
                return std::unexpected(read.error());
            }
            available = *read;
        }
        for (std::size_t i = 0; i < count; ++i) {
            const auto current = static_cast<std::uint32_t>(lba + i);
            std::span<const std::uint8_t, kSectorData> data = kZeroSector;
            if (current < volumeSectors_) {
                if (const auto found = overlays_.find(current); found != overlays_.end()) {
                    data = found->second;
                } else if (i < available) {
                    data = std::span(input).subspan(i * kSectorData).first<kSectorData>();
                } else {
                    return Volume::pastEnd(current);
                }
            }
            SectorEncoder::encode(
                current, data, std::span(encoded).subspan(i * kRawSize).first<kRawSize>());
        }
        // The streams write char.
        file.write(reinterpret_cast<const char *>(encoded.data()),
                   static_cast<std::streamsize>(count * kRawSize));
        if (!file) {
            return ioError(std::format("Cannot write {}.", image.string()));
        }
        lba += static_cast<std::uint32_t>(count);
    }
    file.close();
    if (!file) {
        return ioError(std::format("Cannot write {}.", image.string()));
    }
    if (auto cue = writeFile(output, std::format(kCueTemplate, image.filename().string())); !cue) {
        return std::unexpected(std::move(cue.error()));
    }
    return verify(image).transform([total] { return total; });
}

std::expected<void, Error> ImageBuilder::patchRecord(Volume::Sector &sector,
                                                     std::uint32_t oldLba,
                                                     const std::string &name,
                                                     std::uint32_t newLba,
                                                     std::uint32_t newSize) {
    std::size_t offset = 0;
    while (offset + kRecordName <= sector.size()) {
        const auto length = sector[offset];
        if (length == 0) {
            break;
        }
        const auto nameEnd =
            std::min(offset + kRecordName + sector[offset + kRecordNameLength], sector.size());
        const std::string entryName(sector.begin() +
                                        static_cast<std::ptrdiff_t>(offset + kRecordName),
                                    sector.begin() + static_cast<std::ptrdiff_t>(nameEnd));
        if (readLittle32(sector, offset + kRecordLba) == oldLba &&
            Volume::upperAscii(entryName.substr(0, entryName.find(';'))) == name) {
            writeBoth(sector, offset + kRecordLba, newLba);
            writeBoth(sector, offset + kRecordSize, newSize);
            return {};
        }
        offset += length;
    }
    return discImageError(
        std::format("The directory record of {} is missing from its directory.", name));
}

std::expected<void, Error> ImageBuilder::verify(const std::filesystem::path &image) const {
    std::ifstream file(image, std::ios::binary);
    if (!file) {
        return ioError(std::format("Cannot read {}.", image.string()));
    }
    for (const auto &[target, lba] : written_) {
        const auto &payload =
            std::ranges::find(replacements_, target, &Replacement::target)->payload;
        std::array<char, kVerifyLength> head{};
        file.clear();
        file.seekg(static_cast<std::streamoff>(std::uint64_t{lba} * SectorEncoder::kRawSize +
                                               SectorEncoder::kDataOffset));
        file.read(head.data(), head.size());
        const auto prefix = std::span(payload).first(std::min(payload.size(), kVerifyLength));
        if (static_cast<std::size_t>(file.gcount()) != prefix.size() ||
            !std::ranges::equal(prefix, head, [](auto left, auto right) {
                return left == static_cast<std::uint8_t>(right);
            })) {
            return discImageError(std::format("Verification failed for {}.", target));
        }
    }
    return {};
}
