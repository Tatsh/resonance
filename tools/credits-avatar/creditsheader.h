#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * Compose the credits avatar header.
 *
 * The header defines `RESONANCE_CREDITS_TEXT`, `RESONANCE_CREDITS_AVATAR_SIZE`, and
 * `RESONANCE_CREDITS_HAS_AVATAR`. With texels, `RESONANCE_CREDITS_HAS_AVATAR` is 1 and the header
 * also defines the texel array `kCreditsAvatarTexels`. Without texels,
 * `RESONANCE_CREDITS_HAS_AVATAR` is 0 and the credit has no picture.
 *
 * @param text Credit text.
 * @param size Width and height of the texture in pixels.
 * @param texels RGBA texels, or nothing for a credit without a picture.
 * @return The header text.
 */
std::string composeCreditsHeader(const std::string &text,
                                 int size,
                                 const std::optional<std::vector<std::uint8_t>> &texels);
