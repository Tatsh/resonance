#pragma once

#include "math/color.h"

/**
 * Colour, material, and slice range of one player's tunnel seeker.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from what it drives, the Rnd::TunnelSeeker of index mIndex, and from the
 * alpha fade Update() runs on it.
 *
 * Update() changes the range behind a fade. With a negative mFadeRate the alpha falls, and once it
 * drops below zero Update() hands the stored range to the seeker and fades back in at 0.2 per call,
 * or stops at zero when the stored range is empty. The writer of the range fields and of a
 * negative mFadeRate is not among the recovered routines.
 *
 * TnlPlayer embeds one, 0x28 bytes, at `+0x170`.
 */
class TnlSeekerFade {
public:
    /**
     * Give the seeker its material and colour and apply an empty range.
     *
     * The material is "seeker.mat" in kPlayModeGame and "seeker_loop.mat" in every other play
     * mode. The alpha of color is replaced with 1.
     *
     * @param nIndex The seeker, and the player index.
     * @param color The seeker colour.
     * @ghidraAddress 0x00438308
     */
    TnlSeekerFade(int nIndex, const Color &color);

    /**
     * Apply the stored range to the seeker, or an empty range.
     *
     * @param nActive Non-zero to apply mFirstSlice, mSliceCount, and mRing, zero to apply
     *                `(0, 0, 0)`.
     * @ghidraAddress 0x00454a68
     */
    void SetActive(int nActive);

    /**
     * Advance the alpha by mFadeRate, clamped to 0 through 1.
     *
     * Crossing below zero applies the stored range when mActive is set, and sets mFadeRate to 0.2
     * for a non-empty range and to 0 otherwise. The seeker colour is rewritten only when the alpha
     * changed.
     *
     * @ghidraAddress 0x00454b58
     */
    void Update();

private:
    int mActive;
    int mFirstSlice;
    int mSliceCount;
    int mRing;
    Color mColor;
    float mFadeRate;
    int mIndex;
};
