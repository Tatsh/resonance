#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "isodate.h"
#include "volume.h"

namespace Tools::BuildImage {

/** A file of the original disc and the payload that replaces it. */
struct Replacement {
    std::string target;                /*!< File name without the version suffix. */
    std::vector<std::uint8_t> payload; /*!< Replacement contents. */
    /** Modification time of the payload's source file or archive member, in UTC. */
    Seconds modified;
};

/**
 * Writer of a raw MODE2/2352 image of a volume with replaced files.
 *
 * A payload fitting its original extent overwrites the extent with zero padding. A larger payload
 * moves to sectors appended after the volume, and the volume size grows with it. Either way the
 * directory record of the file gets the payload's extent, its size, and its modification time.
 * Each directory above the file, up to the root, is raised to that time when it is older, and the
 * primary volume descriptor's modification date becomes the root directory's date when that is
 * newer. Every other date is copied from the input. Every output sector is encoded afresh as a
 * Mode 2 Form 1 data sector, and a two-second postgap follows the volume.
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
     * ends early, a file cannot be written, a replaced extent does not open with its payload, or
     * the directory record of a replaced file does not have the payload's extent, size, and date.
     */
    std::expected<std::uint32_t, Error> write(const std::filesystem::path &output);

private:
    // The edit returns true once it has found its record.
    using RecordEdit = std::function<bool(std::span<std::uint8_t>)>;

    ImageBuilder(Volume &source, std::vector<Replacement> replacements);

    std::expected<Volume::Sector *, Error> sector(std::uint32_t lba);
    std::expected<bool, Error> editRecords(const DirectoryExtent &directory,
                                           const RecordEdit &edit);
    std::expected<void, Error> placePayloads();
    std::expected<void, Error>
    patchIso(const LocatedFile &found, const Replacement &replacement, std::uint32_t newLba);
    std::expected<void, Error> patchDescriptor(std::uint32_t volumeSectors);
    [[nodiscard]] std::expected<void, Error> verify(const std::filesystem::path &image) const;

    Volume *source_;
    std::vector<Replacement> replacements_;
    std::map<std::uint32_t, Volume::Sector> overlays_;
    std::uint32_t volumeSectors_ = 0;
    std::vector<std::pair<std::string, std::uint32_t>> written_;
};

} // namespace Tools::BuildImage
