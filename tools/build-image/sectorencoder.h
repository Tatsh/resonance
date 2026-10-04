#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

/** Encoder of CD-ROM XA Mode 2 Form 1 sectors with table-driven EDC and ECC. */
class SectorEncoder {
public:
    /** Data bytes in one sector. */
    static constexpr std::size_t kDataSize = 2048;
    /** Bytes in one raw sector. */
    static constexpr std::size_t kRawSize = 2352;
    /** Offset of the data bytes within a raw Mode 2 Form 1 sector. */
    static constexpr std::size_t kDataOffset = 24;

    /**
     * Encode one data sector with the data subheader.
     *
     * @param lba Sector number relative to the track start.
     * @param data The 2048 data bytes.
     * @param sector Destination of the raw sector with its EDC and both ECC parity blocks.
     */
    static void encode(std::uint32_t lba,
                       std::span<const std::uint8_t, kDataSize> data,
                       std::span<std::uint8_t, kRawSize> sector);
};
