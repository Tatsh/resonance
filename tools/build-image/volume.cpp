#include "volume.h"

#include <algorithm>
#include <format>
#include <regex>
#include <vector>

#include "imageerror.h"

namespace Tools::BuildImage {

std::expected<std::size_t, Error> Volume::readSectors(std::uint32_t lba,
                                                      std::span<std::uint8_t> sectors) {
    const auto count = sectors.size() / kSectorData;
    for (std::size_t i = 0; i < count; ++i) {
        if (auto read = readSector(static_cast<std::uint32_t>(lba + i),
                                   sectors.subspan(i * kSectorData).first<kSectorData>());
            !read) {
            return std::unexpected(std::move(read.error()));
        }
    }
    return count;
}

std::expected<std::string, Error> Volume::bootExecutable() {
    const auto found = find(std::string(kSystemCnf));
    if (!found) {
        return std::unexpected(found.error());
    }
    std::vector<std::uint8_t> data(static_cast<std::size_t>(sectorsFor(found->size)) * kSectorData);
    const auto read = readSectors(found->lba, data);
    if (!read) {
        return std::unexpected(read.error());
    }
    if (*read * kSectorData < data.size()) {
        return pastEnd(found->lba + static_cast<std::uint32_t>(*read));
    }
    data.resize(found->size);
    return parseBoot2(data);
}

std::expected<std::string, Error> Volume::parseBoot2(std::span<const std::uint8_t> data) {
    static const std::regex kBoot2(R"(^\s*BOOT2\s*=\s*cdrom0:\\([^;\s]+))",
                                   std::regex::ECMAScript | std::regex::icase |
                                       std::regex::multiline);
    const std::string text(data.begin(), data.end());
    std::smatch match;
    if (!std::regex_search(text, match, kBoot2)) {
        return discImageError(std::format("{} has no BOOT2 line.", kSystemCnf));
    }
    auto name = upperAscii(match[1].str());
    if (name != kPalExecutable && name != kNtscExecutable) {
        return discImageError(std::format(
            "{} boots {}, an executable of no known FreQuency release.", kSystemCnf, name));
    }
    return name;
}

std::uint32_t Volume::sectorsFor(std::uint64_t size) {
    return static_cast<std::uint32_t>((size + kSectorData - 1) / kSectorData);
}

std::string Volume::upperAscii(std::string_view text) {
    std::string upper(text);
    std::ranges::transform(upper, upper.begin(), [](char c) {
        return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    });
    return upper;
}

std::string Volume::lowerAscii(std::string_view text) {
    std::string lower(text);
    std::ranges::transform(lower, lower.begin(), [](char c) {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    });
    return lower;
}

std::unexpected<Error> Volume::pastEnd(std::uint32_t lba) {
    return discImageError(std::format("Sector {} lies past the end of the medium.", lba));
}

} // namespace Tools::BuildImage
