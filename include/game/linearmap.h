#pragma once

/**
 * Linear map from one integer range onto another, clamped to the output range.
 *
 * The class is not polymorphic and emits no RTTI, and no literal or allocation tag in the image
 * identifies it, so the name is inferred from what the two routines compute. The object is 0x20
 * bytes. PitchPicker's pitch chooser at `0x001c4488` builds one on its stack, storing the four
 * range words directly before calling Init(), which is the shape of an inline constructor around an
 * out-of-line initialiser.
 */
class LinearMap {
public:
    /**
     * Map inMin onto outMin and inMax onto outMax.
     *
     * @param nInMin The start of the input range.
     * @param nInMax The end of the input range.
     * @param nOutMin The value nInMin maps to.
     * @param nOutMax The value nInMax maps to.
     */
    LinearMap(int nInMin, int nInMax, int nOutMin, int nOutMax)
        : mInMin(nInMin), mInMax(nInMax), mOutMin(nOutMin), mOutMax(nOutMax) {
        Init();
    }

    /**
     * Map a value, truncating toward zero and clamping to the output range.
     *
     * @param nValue The input value.
     * @return The mapped value, between the smaller and the larger of the two output ends.
     * @ghidraAddress 0x00536fe0
     */
    int Map(int nValue);

private:
    // Computes mSlope and mOffset in single precision from the four ends, and orders the two
    // output ends into mLower and mUpper.
    // 0x00536f50
    void Init();

    int mInMin;    // +0x00
    int mInMax;    // +0x04
    int mOutMin;   // +0x08
    int mOutMax;   // +0x0c
    int mUpper;    // +0x10, the larger output end
    int mLower;    // +0x14, the smaller output end
    float mSlope;  // +0x18
    float mOffset; // +0x1c
};
