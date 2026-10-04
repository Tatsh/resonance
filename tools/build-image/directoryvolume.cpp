#include "directoryvolume.h"

#include <algorithm>
#include <format>
#include <iterator>
#include <regex>
#include <spdlog/spdlog.h>
#include <system_error>

#include "byteorder.h"
#include "fileio.h"
#include "imageerror.h"

namespace Tools::BuildImage {

namespace {

// The first 12 sectors of a PlayStation 2 disc store the encrypted boot logo the console checks.
constexpr std::uint32_t kLogoSectors = 12;
constexpr std::size_t kSystemAreaSize = kLogoSectors * Volume::kSectorData;
constexpr std::uint32_t kPathTableLba = 18;
constexpr std::uint32_t kPathTableCopies = 4;
constexpr std::size_t kIsoNameMax = 30;
constexpr std::string_view kSystemIdentifier = "PLAYSTATION";
constexpr std::uint8_t kFlagDirectory = 0x02;
constexpr std::uint8_t kTerminatorType = 0xFF;
constexpr int kYearBase = 1900;

// The constants below describe the directory record and path table entry layouts.
constexpr std::size_t kRecordFixedLength = 33;
constexpr std::size_t kRecordLba = 2;
constexpr std::size_t kRecordSize = 10;
constexpr std::size_t kRecordDate = 18;
constexpr std::size_t kRecordFlags = 25;
constexpr std::size_t kRecordVolumeSequence = 28;
constexpr std::size_t kRecordNameLength = 32;
constexpr std::size_t kPathTableEntryFixedLength = 8;

// The constants below describe the primary volume descriptor layout.
constexpr std::size_t kPvdVersion = 6;
constexpr std::size_t kPvdSystemIdentifier = 8;
constexpr std::size_t kPvdVolumeIdentifier = 40;
constexpr std::size_t kPvdIdentifierLength = 32;
constexpr std::size_t kPvdVolumeSectors = 80;
constexpr std::size_t kPvdVolumeSetSize = 120;
constexpr std::size_t kPvdVolumeSequence = 124;
constexpr std::size_t kPvdBlockSize = 128;
constexpr std::size_t kPvdPathTableSize = 132;
constexpr std::size_t kPvdLittlePathTables = 140;
constexpr std::size_t kPvdBigPathTables = 148;
constexpr std::size_t kPvdRootRecord = 156;
constexpr std::size_t kPvdVolumeSetIdentifier = 190;
constexpr std::size_t kPvdApplicationIdentifier = 574;
constexpr std::size_t kPvdApplicationIdentifierLength = 128;
constexpr std::size_t kPvdFileIdentifiers = 702;
constexpr std::size_t kPvdCreationDate = 813;
constexpr std::size_t kPvdModificationDate = 830;
constexpr std::size_t kPvdDateLength = 17;
constexpr std::size_t kPvdDateCount = 3;
constexpr std::size_t kPvdFileStructureVersion = 881;

std::size_t recordLength(std::size_t nameLength) {
    return kRecordFixedLength + nameLength + (1 - nameLength % 2);
}

std::string stripVersion(const std::string &isoName) {
    return isoName.substr(0, isoName.find(';'));
}

std::expected<std::vector<std::filesystem::directory_entry>, Error>
listDirectory(const std::filesystem::path &directory) {
    std::vector<std::filesystem::directory_entry> entries;
    std::error_code error;
    for (std::filesystem::directory_iterator it(directory, error), end; !error && it != end;
         it.increment(error)) {
        entries.push_back(*it);
    }
    if (error) {
        return ioError(std::format("Cannot list {} ({}).", directory.string(), error.message()));
    }
    return entries;
}

std::unexpected<Error> notDiscRoot(const std::filesystem::path &root) {
    return discImageError(std::format("{} lacks {} and is not the root of a PlayStation 2 disc.",
                                      root.string(),
                                      Volume::kSystemCnf));
}

void fill(std::span<std::uint8_t> bytes, std::size_t offset, std::size_t length, char value) {
    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(offset), length, value);
}

void put(std::span<std::uint8_t> bytes, std::size_t offset, std::string_view text) {
    std::ranges::copy(text, bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}

} // namespace

std::expected<std::unique_ptr<DirectoryVolume>, Error>
DirectoryVolume::create(const std::filesystem::path &root,
                        std::optional<std::vector<std::uint8_t>> systemArea) {
    const auto entries = listDirectory(root);
    if (!entries) {
        return std::unexpected(entries.error());
    }
    if (std::ranges::none_of(*entries, [](const auto &entry) {
            return upperAscii(entry.path().filename().string()) == kSystemCnf;
        })) {
        return notDiscRoot(root);
    }
    if (systemArea && systemArea->size() != kSystemAreaSize) {
        return discImageError(std::format("The system area must be {} bytes.", kSystemAreaSize));
    }
    if (!systemArea) {
        spdlog::warn("No system area was given. The boot logo sectors are zero, and a console may "
                     "refuse the disc.");
    }
    std::unique_ptr<DirectoryVolume> volume(new DirectoryVolume());
    volume->systemArea_ =
        std::move(systemArea).value_or(std::vector<std::uint8_t>(kSystemAreaSize));
    if (auto built = volume->build(root); !built) {
        return std::unexpected(std::move(built.error()));
    }
    return volume;
}

std::expected<void, Error> DirectoryVolume::build(const std::filesystem::path &root) {
    // Breadth-first order is path table order. Children sort by identifier.
    std::vector<std::filesystem::path> directories{root};
    std::vector<std::string> directoryNames{std::string(1, '\0')};
    parents_.push_back(0);
    for (std::size_t index = 0; index < directories.size(); ++index) {
        const auto entries = listDirectory(directories[index]);
        if (!entries) {
            return std::unexpected(entries.error());
        }
        std::vector<ListingEntry> listing;
        for (const auto &entry : *entries) {
            auto name = isoName(entry.path());
            if (!name) {
                return std::unexpected(std::move(name.error()));
            }
            std::error_code error;
            listing.push_back({entry.is_directory(error), std::move(*name), entry.path(), 0});
        }
        std::ranges::stable_sort(listing, {}, &ListingEntry::isoName);
        for (auto &entry : listing) {
            if (entry.isDirectory) {
                entry.index = directories.size();
                directories.push_back(entry.path);
                directoryNames.push_back(entry.isoName);
                parents_.push_back(index + 1);
            }
        }
        listings_.push_back(std::move(listing));
    }

    std::uint32_t pathTable = 0;
    for (const auto &name : directoryNames) {
        pathTable += static_cast<std::uint32_t>(kPathTableEntryFixedLength + name.size() +
                                                (name.size() & 1));
    }
    const auto tableSectors = sectorsFor(pathTable);
    auto nextLba = kPathTableLba + kPathTableCopies * tableSectors;
    for (std::size_t i = 0; i < directories.size(); ++i) {
        const auto mtime = modificationTime(directories[i]);
        if (!mtime) {
            return std::unexpected(mtime.error());
        }
        const auto extent = extentSize(listings_[i]);
        directories_.push_back({directoryNames[i], nextLba, *mtime, directories[i], extent});
        nextLba += sectorsFor(extent);
    }
    for (std::size_t i = 0; i < directories.size(); ++i) {
        for (auto &entry : listings_[i]) {
            if (entry.isDirectory) {
                continue;
            }
            std::error_code error;
            const auto size = std::filesystem::file_size(entry.path, error);
            const auto mtime = modificationTime(entry.path);
            if (error) {
                return ioError(
                    std::format("Cannot examine {} ({}).", entry.path.string(), error.message()));
            }
            if (!mtime) {
                return std::unexpected(mtime.error());
            }
            entry.index = files_.size();
            files_.push_back(
                {entry.isoName, nextLba, *mtime, entry.path, static_cast<std::uint32_t>(size)});
            parentOf_.try_emplace(stripVersion(entry.isoName), i);
            fileStarts_.push_back(nextLba);
            nextLba += sectorsFor(size);
        }
    }
    volumeSectors_ = nextLba;

    auto newest = directories_.front().mtime;
    for (const auto &entry : directories_) {
        newest = std::max(newest, entry.mtime);
    }
    for (const auto &entry : files_) {
        newest = std::max(newest, entry.mtime);
    }
    writeDescriptors(pathTable, tableSectors, newest);
    writePathTables(tableSectors);
    for (std::size_t i = 0; i < directories_.size(); ++i) {
        writeDirectory(i);
    }
    return {};
}

std::expected<std::string, Error> DirectoryVolume::identify(const std::filesystem::path &root) {
    const auto entries = listDirectory(root);
    if (!entries) {
        return std::unexpected(entries.error());
    }
    const auto found = std::ranges::find_if(*entries, [](const auto &entry) {
        return upperAscii(entry.path().filename().string()) == kSystemCnf;
    });
    if (found == entries->end()) {
        return notDiscRoot(root);
    }
    return readFile(found->path()).and_then([](const auto &data) { return parseBoot2(data); });
}

std::expected<LocatedFile, Error> DirectoryVolume::find(const std::string &name) {
    for (const auto &entry : files_) {
        if (stripVersion(entry.isoName) == name) {
            const auto &parent = directories_[parentOf_.at(name)];
            return LocatedFile{entry.lba, parent.lba, parent.size, entry.size};
        }
    }
    return discImageError(std::format("{} is missing from the directory.", name));
}

std::expected<void, Error>
DirectoryVolume::readSector(std::uint32_t lba, std::span<std::uint8_t, kSectorData> sector) {
    if (lba < kLogoSectors) {
        std::copy_n(systemArea_.begin() + static_cast<std::ptrdiff_t>(lba * kSectorData),
                    kSectorData,
                    sector.begin());
        return {};
    }
    if (const auto found = metadata_.find(lba); found != metadata_.end()) {
        std::ranges::copy(found->second, sector.begin());
        return {};
    }
    std::ranges::fill(sector, 0);
    const auto position = std::ranges::upper_bound(fileStarts_, lba) - fileStarts_.begin();
    if (position == 0) {
        return {};
    }
    const auto &entry = files_[static_cast<std::size_t>(position - 1)];
    const auto offset = static_cast<std::uint64_t>(lba - entry.lba) * kSectorData;
    if (offset >= entry.size) {
        return {};
    }
    auto [handle, opened] = handles_.try_emplace(entry.path, entry.path, std::ios::binary);
    if (opened && !handle->second) {
        handles_.erase(handle);
        return ioError(std::format("Cannot read {}.", entry.path.string()));
    }
    handle->second.clear();
    handle->second.seekg(static_cast<std::streamoff>(offset));
    // The streams read char.
    handle->second.read(reinterpret_cast<char *>(sector.data()), kSectorData);
    if (handle->second.bad()) {
        return ioError(std::format("Cannot read {}.", entry.path.string()));
    }
    return {};
}

std::uint32_t DirectoryVolume::volumeSectors() const {
    return volumeSectors_;
}

std::expected<std::string, Error> DirectoryVolume::isoName(const std::filesystem::path &entry) {
    static const std::regex kIsoName(R"([A-Z0-9_]+(?:\.[A-Z0-9_]*)?)");
    const auto name = upperAscii(entry.filename().string());
    if (!std::regex_match(name, kIsoName) || name.size() > kIsoNameMax) {
        return discImageError(std::format("{} is not a valid ISO9660 name.", entry.string()));
    }
    std::error_code error;
    return std::filesystem::is_directory(entry, error) ? name : name + ";1";
}

std::expected<DirectoryVolume::Seconds, Error>
DirectoryVolume::modificationTime(const std::filesystem::path &path) {
    std::error_code error;
    const auto written = std::filesystem::last_write_time(path, error);
    if (error) {
        return ioError(std::format("Cannot examine {} ({}).", path.string(), error.message()));
    }
    return std::chrono::floor<std::chrono::seconds>(
        std::chrono::clock_cast<std::chrono::system_clock>(written));
}

std::uint32_t DirectoryVolume::extentSize(const std::vector<ListingEntry> &listing) {
    // The two self and parent records come first. A record never crosses a sector boundary.
    std::size_t used = 0;
    const auto place = [&used](std::size_t nameLength) {
        const auto length = recordLength(nameLength);
        if (used % kSectorData + length > kSectorData) {
            used = (used / kSectorData + 1) * kSectorData;
        }
        used += length;
    };
    place(1);
    place(1);
    for (const auto &entry : listing) {
        place(entry.isoName.size());
    }
    return sectorsFor(used) * static_cast<std::uint32_t>(kSectorData);
}

std::vector<std::uint8_t>
DirectoryVolume::record(std::string_view identifier, const Entry &entry, bool isDirectory) {
    std::vector<std::uint8_t> bytes(recordLength(identifier.size()));
    bytes[0] = static_cast<std::uint8_t>(bytes.size());
    writeBoth(bytes, kRecordLba, entry.lba);
    writeBoth(bytes, kRecordSize, entry.size);
    const std::chrono::year_month_day day{std::chrono::floor<std::chrono::days>(entry.mtime)};
    const std::chrono::hh_mm_ss time{entry.mtime -
                                     std::chrono::floor<std::chrono::days>(entry.mtime)};
    bytes[kRecordDate] = static_cast<std::uint8_t>(static_cast<int>(day.year()) - kYearBase);
    bytes[kRecordDate + 1] = static_cast<std::uint8_t>(static_cast<unsigned>(day.month()));
    bytes[kRecordDate + 2] = static_cast<std::uint8_t>(static_cast<unsigned>(day.day()));
    bytes[kRecordDate + 3] = static_cast<std::uint8_t>(time.hours().count());
    bytes[kRecordDate + 4] = static_cast<std::uint8_t>(time.minutes().count());
    bytes[kRecordDate + 5] = static_cast<std::uint8_t>(time.seconds().count());
    bytes[kRecordFlags] = isDirectory ? kFlagDirectory : 0;
    writeBoth(bytes, kRecordVolumeSequence, 1, 2);
    bytes[kRecordNameLength] = static_cast<std::uint8_t>(identifier.size());
    put(bytes, kRecordFixedLength, identifier);
    return bytes;
}

void DirectoryVolume::writeDescriptors(std::uint32_t pathTable,
                                       std::uint32_t tableSectors,
                                       Seconds newest) {
    Sector descriptor{};
    descriptor[0] = 1;
    put(descriptor, 1, kPvdMagic);
    descriptor[kPvdVersion] = 1;
    fill(descriptor, kPvdSystemIdentifier, kPvdIdentifierLength, ' ');
    put(descriptor, kPvdSystemIdentifier, kSystemIdentifier);
    fill(descriptor, kPvdVolumeIdentifier, kPvdIdentifierLength, ' ');
    writeBoth(descriptor, kPvdVolumeSectors, volumeSectors_);
    writeBoth(descriptor, kPvdVolumeSetSize, 1, 2);
    writeBoth(descriptor, kPvdVolumeSequence, 1, 2);
    writeBoth(descriptor, kPvdBlockSize, kSectorData, 2);
    writeBoth(descriptor, kPvdPathTableSize, pathTable);
    for (std::uint32_t copy = 0; copy < 2; ++copy) {
        writeLittle(
            descriptor, kPvdLittlePathTables + copy * 4, kPathTableLba + copy * tableSectors);
        writeBig(
            descriptor, kPvdBigPathTables + copy * 4, kPathTableLba + (2 + copy) * tableSectors);
    }
    const auto root = record(std::string_view("\0", 1), directories_.front(), true);
    std::ranges::copy(root, descriptor.begin() + kPvdRootRecord);
    fill(descriptor, kPvdVolumeSetIdentifier, kPvdFileIdentifiers - kPvdVolumeSetIdentifier, ' ');
    fill(descriptor, kPvdApplicationIdentifier, kPvdApplicationIdentifierLength, ' ');
    put(descriptor, kPvdApplicationIdentifier, kSystemIdentifier);
    fill(descriptor, kPvdFileIdentifiers, kPvdCreationDate - kPvdFileIdentifiers, ' ');
    put(descriptor, kPvdCreationDate, std::format("{:%Y%m%d%H%M%S}00", newest));
    descriptor[kPvdCreationDate + kPvdDateLength - 1] = 0;
    for (std::size_t date = 0; date < kPvdDateCount; ++date) {
        const auto start = kPvdModificationDate + date * kPvdDateLength;
        fill(descriptor, start, kPvdDateLength - 1, '0');
        descriptor[start + kPvdDateLength - 1] = 0;
    }
    descriptor[kPvdFileStructureVersion] = 1;
    metadata_[kPvdSector] = descriptor;

    Sector terminator{};
    terminator[0] = kTerminatorType;
    put(terminator, 1, kPvdMagic);
    terminator[kPvdVersion] = 1;
    metadata_[kPvdSector + 1] = terminator;
}

void DirectoryVolume::writePathTables(std::uint32_t tableSectors) {
    for (std::uint32_t copy = 0; copy < kPathTableCopies; ++copy) {
        const auto little = copy < 2;
        std::vector<std::uint8_t> table;
        for (std::size_t i = 0; i < directories_.size(); ++i) {
            const auto &identifier = directories_[i].isoName;
            std::array<std::uint8_t, kPathTableEntryFixedLength> fixed{};
            fixed[0] = static_cast<std::uint8_t>(identifier.size());
            const auto parent = static_cast<std::uint32_t>(std::max<std::size_t>(parents_[i], 1));
            if (little) {
                writeLittle(fixed, 2, directories_[i].lba);
                writeLittle(fixed, 6, parent, 2);
            } else {
                writeBig(fixed, 2, directories_[i].lba);
                writeBig(fixed, 6, parent, 2);
            }
            table.insert(table.end(), fixed.begin(), fixed.end());
            table.insert(table.end(), identifier.begin(), identifier.end());
            if (identifier.size() & 1) {
                table.push_back(0);
            }
        }
        table.resize(static_cast<std::size_t>(tableSectors) * kSectorData);
        for (std::uint32_t i = 0; i < tableSectors; ++i) {
            auto &sector = metadata_[kPathTableLba + copy * tableSectors + i];
            std::copy_n(table.begin() + static_cast<std::ptrdiff_t>(i * kSectorData),
                        kSectorData,
                        sector.begin());
        }
    }
}

void DirectoryVolume::writeDirectory(std::size_t directory) {
    const auto &own = directories_[directory];
    const auto &parent = directories_[directory == 0 ? 0 : parents_[directory] - 1];
    std::vector<std::vector<std::uint8_t>> records{record(std::string_view("\0", 1), own, true),
                                                   record(std::string_view("\1", 1), parent, true)};
    for (const auto &entry : listings_[directory]) {
        if (entry.isDirectory) {
            records.push_back(record(entry.isoName, directories_[entry.index], true));
        } else {
            const auto &placed = files_[entry.index];
            records.push_back(record(placed.isoName, placed, false));
        }
    }
    std::vector<std::uint8_t> extent(own.size);
    std::size_t used = 0;
    for (const auto &bytes : records) {
        if (used % kSectorData + bytes.size() > kSectorData) {
            used = (used / kSectorData + 1) * kSectorData;
        }
        std::ranges::copy(bytes, extent.begin() + static_cast<std::ptrdiff_t>(used));
        used += bytes.size();
    }
    for (std::uint32_t i = 0; i < sectorsFor(own.size); ++i) {
        auto &sector = metadata_[own.lba + i];
        std::copy_n(extent.begin() + static_cast<std::ptrdiff_t>(i * kSectorData),
                    kSectorData,
                    sector.begin());
    }
}

} // namespace Tools::BuildImage
