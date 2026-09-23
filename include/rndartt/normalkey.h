#pragma once

#include <vector>

struct Color;

/**
 * Colour recorded as its brightest channel and the three channels divided by it.
 *
 * Two colours of the same hue at different brightness share the three ratios and differ only in
 * mScale. The palette quantiser at `0x00557af8` collects one key per distinct hue, builds a ramp of
 * sixteen shades per key into the destination palette at `0x00557970`, and maps each source pixel
 * to the nearest key's ramp. The class is not polymorphic and emits no RTTI, and no string in the
 * image identifies it. The name follows the analysis program's names for its routines.
 */
struct NormalKey {
    /**
     * Build the key of one colour.
     *
     * The greatest channel becomes mScale and its ratio 1, and a tie goes to the later channel.
     * When blue wins after green was at least red, and blue is zero, the key has a scale of zero
     * and three ratios of 1. Blue winning over a greater red is not tested for zero. The name is
     * the analysis program's.
     *
     * @param flRed The red channel.
     * @param flGreen The green channel.
     * @param flBlue The blue channel.
     * @ghidraAddress 0x00558740
     */
    NormalKey(float flRed, float flGreen, float flBlue);

    /**
     * Add the key of a colour to a set of keys unless the set already has its hue.
     *
     * A key matches when the sum of the absolute differences of the three ratios is under 0.02.
     * The first match is replaced by the new key when the new key is brighter, and otherwise the
     * set is unchanged. Without a match the key is appended. The name is the analysis program's.
     *
     * @param keys The set of keys.
     * @param color The colour. Its alpha is not read.
     * @ghidraAddress 0x00557858
     */
    static void InsertUniqueNormalKey(std::vector<NormalKey> &keys, const Color &color);

    float mScale; /*!< The brightest channel. */
    float mRed;   /*!< Red divided by mScale. */
    float mGreen; /*!< Green divided by mScale. */
    float mBlue;  /*!< Blue divided by mScale. */
};

/**
 * Key the unit's static initialiser constructs from (16, 0, 0).
 *
 * The image does not read it. The name is inferred.
 *
 * @ghidraAddress 0x00725848
 */
extern NormalKey g_normalKeyUnused;
