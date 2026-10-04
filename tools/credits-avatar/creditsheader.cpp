#include "creditsheader.h"

#include <algorithm>
#include <cstddef>
#include <format>

namespace Tools::CreditsAvatar {

namespace {

constexpr std::size_t kBytesPerLine = 16;

// Quotes text as a C string literal, escaping backslashes, quotes, and newlines.
std::string cString(const std::string &text) {
    std::string literal = "\"";
    for (const auto c : text) {
        switch (c) {
        case '\\':
            literal += "\\\\";
            break;
        case '"':
            literal += "\\\"";
            break;
        case '\n':
            literal += "\\n";
            break;
        default:
            literal += c;
            break;
        }
    }
    return literal + '"';
}

} // namespace

std::string composeCreditsHeader(const std::string &text,
                                 int size,
                                 const std::optional<std::vector<std::uint8_t>> &texels) {
    auto header = std::format("#pragma once\n"
                              "\n"
                              "#include <stdint.h>\n"
                              "\n"
                              "#define RESONANCE_CREDITS_TEXT {}\n"
                              "#define RESONANCE_CREDITS_AVATAR_SIZE {}\n"
                              "#define RESONANCE_CREDITS_HAS_AVATAR {}\n",
                              cString(text),
                              size,
                              texels ? 1 : 0);
    if (!texels) {
        return header;
    }
    const auto &bytes = *texels;
    header += "\n"
              "// The texels are RGBA, with alpha on the GS scale.\n"
              "alignas(16) static const uint8_t kCreditsAvatarTexels[] = {\n";
    for (std::size_t i = 0; i < bytes.size(); i += kBytesPerLine) {
        header += "   ";
        for (std::size_t j = i; j < std::min(i + kBytesPerLine, bytes.size()); ++j) {
            header += std::format("{}0x{:02x}", j == i ? " " : ", ", bytes[j]);
        }
        header += ",\n";
    }
    return header + "};\n";
}

} // namespace Tools::CreditsAvatar
