#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>

#include "error.h"

/** Writer of a FreQuency disc image with the built executable and module in place. */
namespace Tools::BuildImage {

/** A file found in the ISO9660 tree. */
struct LocatedFile {
    std::uint32_t lba = 0;        /*!< First data sector, relative to the track start. */
    std::uint32_t parentLba = 0;  /*!< First sector of the containing directory. */
    std::uint32_t parentSize = 0; /*!< Length of the containing directory in bytes. */
    std::uint32_t size = 0;       /*!< Data length in bytes. */
};

/** The data sectors of an ISO9660 volume, from a disc image or built from a disc root. */
class Volume {
public:
    /** Data bytes in one sector. */
    static constexpr std::size_t kSectorData = 2048;
    /** Sector of the primary volume descriptor. */
    static constexpr std::uint32_t kPvdSector = 16;
    /** Identifier of an ISO9660 volume descriptor. */
    static constexpr std::string_view kPvdMagic = "CD001";
    /** Name of the boot configuration file. */
    static constexpr std::string_view kSystemCnf = "SYSTEM.CNF";
    /** Executable of the European release. */
    static constexpr std::string_view kPalExecutable = "SCES_507.91";
    /** Executable of the North American release. */
    static constexpr std::string_view kNtscExecutable = "SCUS_971.25";

    /** The data bytes of one sector. */
    using Sector = std::array<std::uint8_t, kSectorData>;

    /** Release the medium. */
    virtual ~Volume() = default;

    /**
     * Locate a file by name anywhere in the tree.
     *
     * @param name File name without the version suffix.
     * @return Extent and containing directory of the match, or an error when the file is missing
     * or spans several extents.
     */
    virtual std::expected<LocatedFile, Error> find(const std::string &name) = 0;

    /**
     * Read the data bytes of one sector.
     *
     * @param lba Sector number relative to the track start.
     * @param sector Destination of the 2048 data bytes.
     * @return Nothing, or an error when the medium ends before the sector.
     */
    virtual std::expected<void, Error> readSector(std::uint32_t lba,
                                                  std::span<std::uint8_t, kSectorData> sector) = 0;

    /**
     * Read the data bytes of consecutive sectors, stopping where the medium ends.
     *
     * @param lba First sector number relative to the track start.
     * @param sectors Destination, a whole number of sectors long.
     * @return Number of sectors read in full, or an error when a source file cannot be read.
     */
    virtual std::expected<std::size_t, Error> readSectors(std::uint32_t lba,
                                                          std::span<std::uint8_t> sectors);

    /**
     * Report the sector count of the volume.
     *
     * @return Sectors described by the primary volume descriptor.
     */
    [[nodiscard]] virtual std::uint32_t volumeSectors() const = 0;

    /**
     * Read the name of the executable the disc boots.
     *
     * @return File name from the `BOOT2` line of `SYSTEM.CNF`, without the version suffix, or an
     * error when `SYSTEM.CNF` does not boot a known release.
     */
    std::expected<std::string, Error> bootExecutable();

    /**
     * Read the executable name from the text of `SYSTEM.CNF`.
     *
     * @param data Contents of `SYSTEM.CNF`.
     * @return File name from the `BOOT2` line, without the version suffix, or an error when the
     * text does not have a `BOOT2` line or the line identifies an unknown executable.
     */
    static std::expected<std::string, Error> parseBoot2(std::span<const std::uint8_t> data);

    /**
     * Count the sectors that a length in bytes occupies.
     *
     * @param size Length in bytes.
     * @return Sectors, rounded up.
     */
    static std::uint32_t sectorsFor(std::uint64_t size);

    /**
     * Convert the ASCII letters of a text to upper case.
     *
     * @param text Text to convert.
     * @return The text with each ASCII letter in upper case.
     */
    static std::string upperAscii(std::string_view text);

    /**
     * Convert the ASCII letters of a text to lower case.
     *
     * @param text Text to convert.
     * @return The text with each ASCII letter in lower case.
     */
    static std::string lowerAscii(std::string_view text);

    /**
     * Report that a sector lies past the end of the medium.
     *
     * @param lba Sector number relative to the track start.
     * @return The error.
     */
    static std::unexpected<Error> pastEnd(std::uint32_t lba);
};

} // namespace Tools::BuildImage
