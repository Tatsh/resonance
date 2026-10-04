#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "volume.h"

namespace Tools::BuildImage {

/** Random access to the data track of a cue, raw, or ISO disc image. */
class SourceImage final : public Volume {
public:
    /**
     * Open an image and verify its primary volume descriptor.
     *
     * @param path Cue, raw, or ISO image path.
     * @return The open image, or an error when the image cannot be opened, the layout is unknown,
     * or sector sixteen lacks the descriptor.
     */
    static std::expected<std::unique_ptr<SourceImage>, Error>
    open(const std::filesystem::path &path);

    /** @copydoc Volume::find */
    std::expected<LocatedFile, Error> find(const std::string &name) override;
    /** @copydoc Volume::readSector */
    std::expected<void, Error> readSector(std::uint32_t lba,
                                          std::span<std::uint8_t, kSectorData> sector) override;
    /** @copydoc Volume::readSectors */
    std::expected<std::size_t, Error> readSectors(std::uint32_t lba,
                                                  std::span<std::uint8_t> sectors) override;
    /** @copydoc Volume::volumeSectors */
    [[nodiscard]] std::uint32_t volumeSectors() const override;

private:
    struct Geometry {
        std::size_t dataOffset = 0;
        std::size_t sectorSize = 0;
        std::uint32_t trackStart = 0;
    };

    struct Record {
        bool isDirectory = false;
        std::uint32_t lba = 0;
        bool multiExtent = false;
        std::string name;
        std::uint32_t size = 0;
    };

    using Layout = std::pair<std::filesystem::path, Geometry>;

    SourceImage(std::ifstream handle, Geometry geometry);

    static std::expected<Layout, Error> detectGeometry(const std::filesystem::path &path);
    static std::expected<Layout, Error> parseCue(const std::filesystem::path &path);
    static std::expected<std::filesystem::path, Error>
    resolveBinPath(const std::filesystem::path &directory, const std::string &binName);
    std::expected<std::vector<Record>, Error> records(std::uint32_t lba, std::uint32_t size);

    std::ifstream handle_;
    Geometry geometry_;
    std::uint32_t volumeSectors_ = 0;
    std::vector<char> rawBuffer_;
};

} // namespace Tools::BuildImage
