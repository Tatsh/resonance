#pragma once

#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <system_error>

#include "imageerror.h"

namespace Tools::BuildImage {

/** A UTC time with a resolution of one second. */
using Seconds = std::chrono::sys_seconds;

/** Minutes in one step of an ISO9660 zone offset. */
constexpr int kIsoZoneMinutes = 15;

/**
 * Write the seven-byte date of a directory record, in UTC.
 *
 * @param bytes Buffer holding the record.
 * @param offset Offset of the date's first byte.
 * @param time Time to write.
 */
inline void writeRecordDate(std::span<std::uint8_t> bytes, std::size_t offset, Seconds time) {
    constexpr int kYearBase = 1900;
    const auto midnight = std::chrono::floor<std::chrono::days>(time);
    const std::chrono::year_month_day day{midnight};
    const std::chrono::hh_mm_ss clock{time - midnight};
    bytes[offset] = static_cast<std::uint8_t>(static_cast<int>(day.year()) - kYearBase);
    bytes[offset + 1] = static_cast<std::uint8_t>(static_cast<unsigned>(day.month()));
    bytes[offset + 2] = static_cast<std::uint8_t>(static_cast<unsigned>(day.day()));
    bytes[offset + 3] = static_cast<std::uint8_t>(clock.hours().count());
    bytes[offset + 4] = static_cast<std::uint8_t>(clock.minutes().count());
    bytes[offset + 5] = static_cast<std::uint8_t>(clock.seconds().count());
    bytes[offset + 6] = 0;
}

/**
 * Read the seven-byte date of a directory record.
 *
 * @param bytes Buffer holding the record.
 * @param offset Offset of the date's first byte.
 * @return The date in UTC, with the record's zone offset removed.
 */
inline Seconds readRecordDate(std::span<const std::uint8_t> bytes, std::size_t offset) {
    constexpr int kYearBase = 1900;
    const std::chrono::year_month_day day{std::chrono::year{kYearBase + bytes[offset]},
                                          std::chrono::month{bytes[offset + 1]},
                                          std::chrono::day{bytes[offset + 2]}};
    return std::chrono::sys_days{day} + std::chrono::hours{bytes[offset + 3]} +
           std::chrono::minutes{bytes[offset + 4]} + std::chrono::seconds{bytes[offset + 5]} -
           std::chrono::minutes{kIsoZoneMinutes * static_cast<std::int8_t>(bytes[offset + 6])};
}

/**
 * Format the 16 digits of a volume descriptor date, in UTC with hundredths of zero.
 *
 * @param time Time to format.
 * @return The digits; the descriptor's 17th byte, the zone offset, is written separately.
 */
inline std::string volumeDate(Seconds time) {
    return std::format("{:%Y%m%d%H%M%S}00", time);
}

/**
 * Read the 17-byte date of a volume descriptor.
 *
 * @param bytes Buffer holding the descriptor.
 * @param offset Offset of the date's first digit.
 * @return The date in UTC, or nothing when the date is unset (all digits zero) or not digits.
 */
inline std::optional<Seconds> readVolumeDate(std::span<const std::uint8_t> bytes,
                                             std::size_t offset) {
    constexpr std::size_t kFields[] = {4, 2, 2, 2, 2, 2};
    constexpr std::size_t kZone = 16;
    int values[std::size(kFields)] = {};
    auto position = offset;
    for (std::size_t i = 0; i < std::size(kFields); ++i) {
        // The descriptor stores ASCII digits.
        const auto *first = reinterpret_cast<const char *>(bytes.data() + position);
        const auto [last, error] = std::from_chars(first, first + kFields[i], values[i]);
        if (error != std::errc() || last != first + kFields[i]) {
            return std::nullopt;
        }
        position += kFields[i];
    }
    if (values[0] == 0) {
        return std::nullopt;
    }
    const std::chrono::year_month_day day{std::chrono::year{values[0]},
                                          std::chrono::month{static_cast<unsigned>(values[1])},
                                          std::chrono::day{static_cast<unsigned>(values[2])}};
    return std::chrono::sys_days{day} + std::chrono::hours{values[3]} +
           std::chrono::minutes{values[4]} + std::chrono::seconds{values[5]} -
           std::chrono::minutes{kIsoZoneMinutes * static_cast<std::int8_t>(bytes[offset + kZone])};
}

/**
 * Read the modification time of a file or directory.
 *
 * @param path File or directory to examine.
 * @return The time in UTC, or an error when the path cannot be examined.
 */
inline std::expected<Seconds, Error> fileModificationTime(const std::filesystem::path &path) {
    std::error_code error;
    const auto written = std::filesystem::last_write_time(path, error);
    if (error) {
        return ioError(std::format("Cannot examine {} ({}).", path.string(), error.message()));
    }
    return std::chrono::floor<std::chrono::seconds>(
        std::chrono::clock_cast<std::chrono::system_clock>(written));
}

} // namespace Tools::BuildImage
