#include "sectorencoder.h"

#include <algorithm>
#include <array>

namespace {

constexpr std::uint32_t kLeadInSectors = 150;
constexpr std::uint32_t kSectorsPerSecond = 75;
constexpr std::uint32_t kSecondsPerMinute = 60;
constexpr std::uint8_t kMode2 = 2;
constexpr std::uint32_t kEccPolynomial = 0x11D;
constexpr std::uint32_t kEdcPolynomial = 0xD8018001;
constexpr std::array<std::uint8_t, 12> kSync{
    0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0};
constexpr std::array<std::uint8_t, 8> kDataSubheader{0, 0, 8, 0, 0, 0, 8, 0};

constexpr std::size_t kHeaderOffset = 12;
constexpr std::size_t kSubheaderOffset = 16;
constexpr std::size_t kEdcOffset = 2072;
constexpr std::size_t kParityPOffset = 2076;
constexpr std::size_t kParityQOffset = 2248;
constexpr std::size_t kParityPColumns = 86;
constexpr std::size_t kParityPRows = 24;
constexpr std::size_t kParityQDiagonals = 52;
constexpr std::size_t kParityQRows = 43;
constexpr std::size_t kParityQStride = 88;

struct EccTables {
    std::array<std::uint8_t, 256> forward{};
    std::array<std::uint8_t, 256> backward{};
};

constexpr EccTables buildEccTables() {
    EccTables tables;
    for (std::uint32_t value = 0; value < 256; ++value) {
        const auto doubled =
            static_cast<std::uint8_t>((value << 1) ^ ((value & 0x80) ? kEccPolynomial : 0));
        tables.forward[value] = doubled;
        tables.backward[value ^ doubled] = static_cast<std::uint8_t>(value);
    }
    return tables;
}

constexpr std::array<std::uint32_t, 256> buildEdcTable() {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t value = 0; value < 256; ++value) {
        auto edc = value;
        for (int bit = 0; bit < 8; ++bit) {
            edc = (edc >> 1) ^ ((edc & 1) ? kEdcPolynomial : 0);
        }
        table[value] = edc;
    }
    return table;
}

constexpr auto kEccTables = buildEccTables();
constexpr auto kEdcTable = buildEdcTable();

constexpr std::uint8_t bcd(std::uint32_t value) {
    return static_cast<std::uint8_t>(((value / 10) << 4) | (value % 10));
}

// Each major index is one parity column (P) or diagonal (Q); its bytes are `minorCount` steps of
// `minorStep` through `source`, wrapping at the end of the protected area.
void computeParity(const std::uint8_t *source,
                   std::size_t majorCount,
                   std::size_t minorCount,
                   std::size_t majorStep,
                   std::size_t minorStep,
                   std::uint8_t *destination) {
    const auto size = majorCount * minorCount;
    for (std::size_t major = 0; major < majorCount; ++major) {
        auto index = (major >> 1) * majorStep + (major & 1);
        std::uint8_t first = 0;
        std::uint8_t second = 0;
        for (std::size_t minor = 0; minor < minorCount; ++minor) {
            const auto value = source[index];
            index += minorStep;
            if (index >= size) {
                index -= size;
            }
            first = kEccTables.forward[first ^ value];
            second ^= value;
        }
        first = kEccTables.backward[kEccTables.forward[first] ^ second];
        destination[major] = first;
        destination[major + majorCount] = first ^ second;
    }
}

} // namespace

void SectorEncoder::encode(std::uint32_t lba,
                           std::span<const std::uint8_t, kDataSize> data,
                           std::span<std::uint8_t, kRawSize> sector) {
    auto *raw = sector.data();
    std::ranges::copy(kSync, raw);
    std::ranges::copy(kDataSubheader, raw + kSubheaderOffset);
    std::ranges::copy(data, raw + kDataOffset);

    std::uint32_t edc = 0;
    for (auto i = kSubheaderOffset; i < kEdcOffset; ++i) {
        edc = (edc >> 8) ^ kEdcTable[(edc ^ raw[i]) & 0xFF];
    }
    for (std::size_t i = 0; i < 4; ++i) {
        raw[kEdcOffset + i] = static_cast<std::uint8_t>(edc >> (8 * i));
    }

    // Form 1 parity treats the header as zero.
    std::fill_n(raw + kHeaderOffset, 4, 0);
    computeParity(raw + kHeaderOffset,
                  kParityPColumns,
                  kParityPRows,
                  2,
                  kParityPColumns,
                  raw + kParityPOffset);
    computeParity(raw + kHeaderOffset,
                  kParityQDiagonals,
                  kParityQRows,
                  kParityPColumns,
                  kParityQStride,
                  raw + kParityQOffset);

    const auto address = lba + kLeadInSectors;
    raw[kHeaderOffset] = bcd(address / (kSectorsPerSecond * kSecondsPerMinute));
    raw[kHeaderOffset + 1] = bcd(address / kSectorsPerSecond % kSecondsPerMinute);
    raw[kHeaderOffset + 2] = bcd(address % kSectorsPerSecond);
    raw[kHeaderOffset + 3] = kMode2;
}
