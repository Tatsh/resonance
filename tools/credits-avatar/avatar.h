#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/** Generator of the credits avatar header. */
namespace Tools::CreditsAvatar {

/**
 * Fetch a GitHub avatar over HTTPS and convert it to RGBA texels with ImageMagick.
 *
 * Alpha is on the GS scale, where 0x80 is opaque. Each failure logs a warning and returns no
 * texels.
 *
 * @param user GitHub user name.
 * @param size Width and height of the texture in pixels.
 * @param magick ImageMagick command, or empty when ImageMagick is not installed.
 * @return The texels, or nothing when the avatar cannot be fetched or converted.
 */
std::optional<std::vector<std::uint8_t>>
fetchAvatar(const std::string &user, int size, const std::string &magick);

} // namespace Tools::CreditsAvatar
