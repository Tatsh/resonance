#include "imagebuilder.h"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <span>
#include <string_view>

#include <spdlog/spdlog.h>

#include "byteorder.h"
#include "fileio.h"
#include "imageerror.h"
#include "sectorencoder.h"
#include "sourceimage.h"

namespace Tools::BuildImage {

namespace {

constexpr std::uint32_t kPostgapSectors = 150;
constexpr std::size_t kBatchSectors = 1024;
constexpr std::string_view kElfMagic = "\x7f"
                                       "ELF";
constexpr std::size_t kVerifyLength = 4;
constexpr std::size_t kPvdVolumeSectors = 80;
constexpr std::size_t kPvdRootRecord = 156;
constexpr std::size_t kPvdModificationDate = 830;

// The offsets below are within a directory record.
constexpr std::size_t kRecordLba = 2;
constexpr std::size_t kRecordSize = 10;
constexpr std::size_t kRecordDate = 18;
constexpr std::size_t kRecordNameLength = 32;
constexpr std::size_t kRecordName = 33;

// The self and parent records open every directory extent.
constexpr int kDirectoryOwnRecords = 2;

constexpr std::string_view kCueTemplate =
    "FILE \"{}\" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n";

void raiseRecord(std::span<std::uint8_t> record, Seconds time) {
    if (readRecordDate(record, kRecordDate) < time) {
        writeRecordDate(record, kRecordDate, time);
    }
}

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
    for (const auto &replacement : replacements_) {
        const auto &[target, payload, modified] = replacement;
        if (!std::string_view(reinterpret_cast<const char *>(payload.data()), payload.size())
                 .starts_with(kElfMagic)) {
            spdlog::warn("Payload for `{}` lacks an ELF header.", target);
        }
        const auto found = source_->find(target);
        if (!found) {
            return std::unexpected(found.error());
        }
        auto newLba = found->lba;
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
            spdlog::info("Replaced {} in place ({} bytes).", target, payload.size());
        } else {
            newLba = appendLba;
            const auto sectors = Volume::sectorsFor(payload.size());
            for (std::uint32_t index = 0; index < sectors; ++index) {
                Volume::Sector chunk{};
                const auto slice = std::span(payload).subspan(std::size_t{index} * kSectorData);
                std::ranges::copy(slice.first(std::min(slice.size(), kSectorData)), chunk.begin());
                overlays_[appendLba + index] = chunk;
            }
            spdlog::info("Relocated {} to sector {} ({} bytes).", target, newLba, payload.size());
            appendLba += sectors;
        }
        if (auto patched = patchIso(*found, replacement, newLba); !patched) {
            return patched;
        }
        written_.emplace_back(target, newLba);
    }
    if (auto patched = patchDescriptor(appendLba); !patched) {
        return patched;
    }
    volumeSectors_ = appendLba;
    return {};
}

std::expected<Volume::Sector *, Error> ImageBuilder::sector(std::uint32_t lba) {
    if (const auto found = overlays_.find(lba); found != overlays_.end()) {
        return &found->second;
    }
    Volume::Sector read;
    if (auto status = source_->readSector(lba, read); !status) {
        return std::unexpected(std::move(status.error()));
    }
    return &overlays_.emplace(lba, read).first->second;
}

std::expected<bool, Error> ImageBuilder::editRecords(const DirectoryExtent &directory,
                                                     const RecordEdit &edit) {
    for (std::uint32_t index = 0; index < Volume::sectorsFor(directory.size); ++index) {
        const auto found = sector(directory.lba + index);
        if (!found) {
            return std::unexpected(found.error());
        }
        auto &bytes = **found;
        std::size_t offset = 0;
        while (offset + kRecordName <= bytes.size() && bytes[offset] != 0) {
            const auto length = bytes[offset];
            if (edit(std::span(bytes).subspan(
                    offset, std::min<std::size_t>(length, bytes.size() - offset)))) {
                return true;
            }
            offset += length;
        }
    }
    return false;
}

std::expected<void, Error> ImageBuilder::patchIso(const LocatedFile &found,
                                                  const Replacement &replacement,
                                                  std::uint32_t newLba) {
    const auto &[name, payload, modified] = replacement;
    auto patched = editRecords(found.directories.front(), [&](std::span<std::uint8_t> record) {
        const auto nameEnd =
            std::min<std::size_t>(kRecordName + record[kRecordNameLength], record.size());
        const std::string entryName(record.begin() + static_cast<std::ptrdiff_t>(kRecordName),
                                    record.begin() + static_cast<std::ptrdiff_t>(nameEnd));
        if (readLittle32(record, kRecordLba) != found.lba ||
            Volume::upperAscii(entryName.substr(0, entryName.find(';'))) != name) {
            return false;
        }
        writeBoth(record, kRecordLba, newLba);
        writeBoth(record, kRecordSize, static_cast<std::uint32_t>(payload.size()));
        writeRecordDate(record, kRecordDate, modified);
        return true;
    });
    if (patched && !*patched) {
        return discImageError(
            std::format("The directory record of {} is missing from its directory.", name));
    }
    for (std::size_t level = 0; patched && level < found.directories.size(); ++level) {
        const auto &directory = found.directories[level];
        auto own = 0;
        patched = editRecords(directory, [&](std::span<std::uint8_t> record) {
            raiseRecord(record, modified);
            return ++own == kDirectoryOwnRecords;
        });
        if (!patched) {
            break;
        }
        if (level + 1 < found.directories.size()) {
            patched =
                editRecords(found.directories[level + 1], [&](std::span<std::uint8_t> record) {
                    if (readLittle32(record, kRecordLba) != directory.lba) {
                        return false;
                    }
                    raiseRecord(record, modified);
                    return true;
                });
            if (patched && !*patched) {
                return discImageError(std::format(
                    "The directory at sector {} is missing from its parent.", directory.lba));
            }
        } else {
            const auto descriptor = sector(Volume::kPvdSector);
            if (!descriptor) {
                return std::unexpected(descriptor.error());
            }
            raiseRecord(std::span(**descriptor).subspan(kPvdRootRecord), modified);
        }
    }
    return patched.transform([](bool) {});
}

std::expected<void, Error> ImageBuilder::patchDescriptor(std::uint32_t volumeSectors) {
    const auto found = sector(Volume::kPvdSector);
    if (!found) {
        return std::unexpected(found.error());
    }
    auto &descriptor = **found;
    writeLittle(descriptor, kPvdVolumeSectors, volumeSectors);
    writeBig(descriptor, kPvdVolumeSectors + 4, volumeSectors);
    const auto root = readRecordDate(descriptor, kPvdRootRecord + kRecordDate);
    if (const auto current = readVolumeDate(descriptor, kPvdModificationDate);
        !current || *current < root) {
        const auto date = volumeDate(root);
        std::ranges::copy(date, descriptor.begin() + kPvdModificationDate);
        descriptor[kPvdModificationDate + date.size()] = 0;
    }
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

std::expected<void, Error> ImageBuilder::verify(const std::filesystem::path &image) const {
    std::ifstream file(image, std::ios::binary);
    if (!file) {
        return ioError(std::format("Cannot read {}.", image.string()));
    }
    auto written = SourceImage::open(image);
    if (!written) {
        return std::unexpected(written.error());
    }
    for (const auto &[target, lba] : written_) {
        const auto &replacement = *std::ranges::find(replacements_, target, &Replacement::target);
        const auto &payload = replacement.payload;
        const auto record = (*written)->find(target);
        if (!record || record->lba != lba || record->size != payload.size() ||
            record->modified != replacement.modified) {
            return discImageError(
                std::format("The directory record of {} does not match its payload.", target));
        }
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

} // namespace Tools::BuildImage
