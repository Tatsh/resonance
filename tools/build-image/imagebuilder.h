#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "volume.h"

/** A file of the original disc and the payload that replaces it. */
struct Replacement {
    std::string target;                /*!< File name without the version suffix. */
    std::vector<std::uint8_t> payload; /*!< Replacement contents. */
};

/**
 * Writer of a raw MODE2/2352 image of a volume with replaced files.
 *
 * A payload fitting its original extent overwrites the extent with zero padding, and the metadata
 * is unchanged. A larger payload moves to sectors appended after the volume, and the directory
 * record and volume size move with the payload. Every output sector is encoded afresh as a Mode 2
 * Form 1 data sector, and a two-second postgap follows the volume.
 */
class ImageBuilder {
public:
    /**
     * Place each payload and collect the sectors that change.
     *
     * @param source Open original image or disc root. It must outlive the builder.
     * @param replacements Payloads in placement order.
     * @return The planned image, or an error when a target is missing or its directory record
     * cannot be patched.
     */
    static std::expected<ImageBuilder, Error> plan(Volume &source,
                                                   std::vector<Replacement> replacements);

    /**
     * Encode the raw image, write its cue sheet, and check the replaced extents.
     *
     * @param output Destination cue sheet path. The image is written beside it with a `.bin`
     * suffix.
     * @return Sectors in the output image, including the postgap, or an error when the source
     * ends early, a file cannot be written, or a replaced extent does not open with its payload.
     */
    std::expected<std::uint32_t, Error> write(const std::filesystem::path &output);

private:
    ImageBuilder(Volume &source, std::vector<Replacement> replacements);

    std::expected<void, Error> placePayloads();
    static std::expected<void, Error> patchRecord(Volume::Sector &sector,
                                                  std::uint32_t oldLba,
                                                  const std::string &name,
                                                  std::uint32_t newLba,
                                                  std::uint32_t newSize);
    [[nodiscard]] std::expected<void, Error> verify(const std::filesystem::path &image) const;

    Volume *source_;
    std::vector<Replacement> replacements_;
    std::map<std::uint32_t, Volume::Sector> overlays_;
    std::uint32_t volumeSectors_ = 0;
    std::vector<std::pair<std::string, std::uint32_t>> written_;
};
