#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "volume.h"

namespace Tools::BuildImage {

/**
 * An ISO9660 volume built from the files of a disc root directory.
 *
 * The primary volume descriptor, the terminator, and the path tables take the sectors the original
 * disc gives them. The directories follow in path table order, and then the files in the same
 * order. Metadata sectors are built in memory, and file sectors are read from the source files on
 * demand.
 */
class DirectoryVolume final : public Volume {
public:
    /**
     * Build the volume layout.
     *
     * @param root Disc root directory with `SYSTEM.CNF`.
     * @param systemArea The 12 sectors of boot logo data. The sectors are zero when omitted.
     * @return The volume, or an error when the directory is not a disc root, a name is not valid,
     * a file cannot be examined, or the system area has the wrong size.
     */
    static std::expected<std::unique_ptr<DirectoryVolume>, Error>
    create(const std::filesystem::path &root, std::optional<std::vector<std::uint8_t>> systemArea);

    /**
     * Read the name of the executable a disc root boots, without building the volume.
     *
     * @param root Disc root directory with `SYSTEM.CNF`.
     * @return File name from the `BOOT2` line of `SYSTEM.CNF`, without the version suffix, or an
     * error when the directory lacks `SYSTEM.CNF` or `SYSTEM.CNF` does not boot a known release.
     */
    static std::expected<std::string, Error> identify(const std::filesystem::path &root);

    /** @copydoc Volume::find */
    std::expected<LocatedFile, Error> find(const std::string &name) override;
    /** @copydoc Volume::readSector */
    std::expected<void, Error> readSector(std::uint32_t lba,
                                          std::span<std::uint8_t, kSectorData> sector) override;
    /** @copydoc Volume::volumeSectors */
    [[nodiscard]] std::uint32_t volumeSectors() const override;

private:
    using Seconds = std::chrono::sys_seconds;

    struct Entry {
        std::string isoName;
        std::uint32_t lba = 0;
        Seconds mtime;
        std::filesystem::path path;
        std::uint32_t size = 0;
    };

    struct ListingEntry {
        bool isDirectory = false;
        std::string isoName;
        std::filesystem::path path;
        std::size_t index = 0;
    };

    DirectoryVolume() = default;

    std::expected<void, Error> build(const std::filesystem::path &root);
    static std::expected<std::string, Error> isoName(const std::filesystem::path &entry);
    static std::expected<Seconds, Error> modificationTime(const std::filesystem::path &path);
    static std::uint32_t extentSize(const std::vector<ListingEntry> &listing);
    static std::vector<std::uint8_t>
    record(std::string_view identifier, const Entry &entry, bool isDirectory);
    void writeDescriptors(std::uint32_t pathTable, std::uint32_t tableSectors, Seconds newest);
    void writePathTables(std::uint32_t tableSectors);
    void writeDirectory(std::size_t directory);

    std::vector<std::uint8_t> systemArea_;
    std::vector<Entry> directories_;
    std::vector<std::size_t> parents_;
    std::vector<std::vector<ListingEntry>> listings_;
    std::vector<Entry> files_;
    std::map<std::string, std::size_t> parentOf_;
    std::vector<std::uint32_t> fileStarts_;
    std::map<std::uint32_t, Sector> metadata_;
    std::map<std::filesystem::path, std::ifstream> handles_;
    std::uint32_t volumeSectors_ = 0;
};

} // namespace Tools::BuildImage
