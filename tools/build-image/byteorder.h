#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Tools::BuildImage {

/**
 * Read a little-endian 16-bit value.
 *
 * @param bytes Buffer to read.
 * @param offset Offset of the value's first byte.
 * @return The value.
 */
inline std::uint16_t readLittle16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(bytes[offset] | (bytes[offset + 1] << 8));
}

/**
 * Read a little-endian 32-bit value.
 *
 * @param bytes Buffer to read.
 * @param offset Offset of the value's first byte.
 * @return The value.
 */
inline std::uint32_t readLittle32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

/**
 * Write a little-endian value.
 *
 * @param bytes Buffer to write.
 * @param offset Offset of the value's first byte.
 * @param value Value to write.
 * @param width Width of the value in bytes.
 */
inline void writeLittle(std::span<std::uint8_t> bytes,
                        std::size_t offset,
                        std::uint32_t value,
                        std::size_t width = 4) {
    for (std::size_t i = 0; i < width; ++i) {
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
    }
}

/**
 * Write a big-endian value.
 *
 * @param bytes Buffer to write.
 * @param offset Offset of the value's first byte.
 * @param value Value to write.
 * @param width Width of the value in bytes.
 */
inline void writeBig(std::span<std::uint8_t> bytes,
                     std::size_t offset,
                     std::uint32_t value,
                     std::size_t width = 4) {
    for (std::size_t i = 0; i < width; ++i) {
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (8 * (width - 1 - i)));
    }
}

/**
 * Write a value in the ISO9660 both-endian form, little-endian followed by big-endian.
 *
 * @param bytes Buffer to write.
 * @param offset Offset of the little-endian copy.
 * @param value Value to write.
 * @param width Width of one copy in bytes.
 */
inline void writeBoth(std::span<std::uint8_t> bytes,
                      std::size_t offset,
                      std::uint32_t value,
                      std::size_t width = 4) {
    writeLittle(bytes, offset, value, width);
    writeBig(bytes, offset + width, value, width);
}

} // namespace Tools::BuildImage
