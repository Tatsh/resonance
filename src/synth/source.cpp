#include "synth/source.h"

#include <cmath>

namespace {

constexpr float kTwoPi = 6.2831855f;

// The sine wave is scaled and offset from [-1, 1] into [0, 1].
constexpr float kSineHalfRange = 0.5f;

// The triangle's rising sawtooth spans two units, and the second is folded back down.
constexpr float kTriangleSpan = 2.0f;

class Sine : public Source {
public:
    Sine(float flRate, float flPhase) : mRate(flRate), mPhase(flPhase) {
    }

    // NTSC-U/C: 0x00546198, PAL: 0x005866c8
    virtual int GetValue(float flTime, float *pValue) {
        *pValue = (std::sin((mRate * flTime) + mPhase) * kSineHalfRange) + kSineHalfRange;
        return 1;
    }

private:
    float mRate;  // Radians per unit of time.
    float mPhase; // Radians.
};

class Square : public Source {
public:
    explicit Square(float flPeriod) : mPeriod(flPeriod) {
    }

    // NTSC-U/C: 0x00546290, PAL: 0x005867c0
    virtual int GetValue(float flTime, float *pValue) {
        // The binary compares with 0.5 rather than half the period.
        *pValue = std::fmod(flTime, mPeriod) < 0.5 ? 1.0f : 0.0f;
        return 1;
    }

private:
    float mPeriod;
};

class Tri : public Source {
public:
    Tri(float flPeriod, float flOffset, float flScale)
        : mPeriod(flPeriod), mOffset(flOffset), mScale(flScale) {
    }

    // NTSC-U/C: 0x00546398, PAL: 0x005868c8
    virtual int GetValue(float flTime, float *pValue) {
        *pValue = std::fmod(flTime + mOffset, mPeriod) * mScale;
        if (*pValue > 1.0) {
            *pValue = kTriangleSpan - *pValue;
        }
        return 1;
    }

private:
    float mPeriod;
    float mOffset; // The starting phase, in units of time.
    float mScale;
};

class Ramp : public Source {
public:
    Ramp(float flPeriod, float flOffset, float flScale)
        : mPeriod(flPeriod), mOffset(flOffset), mScale(flScale) {
    }

    // NTSC-U/C: 0x005464c0, PAL: 0x005869f0
    virtual int GetValue(float flTime, float *pValue) {
        *pValue = std::fmod(flTime + mOffset, mPeriod) * mScale;
        return 1;
    }

private:
    float mPeriod;
    float mOffset; // The starting phase, in units of time.
    float mScale;
};

class Fade : public Source {
public:
    Fade(float flDuration, float flTarget, int bStopAtEnd)
        : mDuration(flDuration), mTarget(flTarget), mStopAtEnd(bStopAtEnd) {
    }

    // NTSC-U/C: 0x005465b8, PAL: 0x00586ae8
    virtual int GetValue(float flTime, float *pValue) {
        if (mDuration < flTime) {
            *pValue = mTarget;
            return mStopAtEnd == 0;
        }
        if (mTarget == 0.0) {
            *pValue = 1.0f - (flTime / mDuration);
        } else {
            *pValue = flTime / mDuration;
        }
        return 1;
    }

private:
    float mDuration;
    float mTarget; // 0 or 1, the value the fade ends on.
    int mStopAtEnd;
};

class HoldAndFadeDown : public Source {
public:
    HoldAndFadeDown(float flHold, float flFade, int bStopAtEnd)
        : mHold(flHold), mFade(flFade), mStopAtEnd(bStopAtEnd) {
    }

    // NTSC-U/C: 0x00546708, PAL: 0x00586c38
    virtual int GetValue(float flTime, float *pValue) {
        if (flTime <= mHold) {
            *pValue = 1.0f;
            return 1;
        }
        if ((mHold + mFade) <= flTime) {
            *pValue = 0.0f;
            return mStopAtEnd ^ 1;
        }
        *pValue = ((mHold - flTime) / mFade) + 1.0f;
        return 1;
    }

private:
    float mHold;
    float mFade;
    int mStopAtEnd;
};

} // namespace

Source *Source::AllocateSineSource(float flPeriod, float flPhase) {
    return new Sine(kTwoPi / flPeriod, flPhase * kTwoPi);
}

Source *Source::AllocateSquareSource(float flPeriod) {
    return new Square(flPeriod);
}

Source *Source::AllocateTriSource(float flPeriod, float flPhase) {
    return new Tri(flPeriod, flPhase * flPeriod, kTriangleSpan / flPeriod);
}

Source *Source::AllocateRampSource(float flPeriod, float flPhase) {
    return new Ramp(flPeriod, flPhase * flPeriod, 1.0f / flPeriod);
}

Source *Source::AllocateFadeOutSource(int bStopAtEnd, float flDuration) {
    return new Fade(flDuration, 0.0f, bStopAtEnd);
}

Source *Source::AllocateFadeInSource(int bStopAtEnd, float flDuration) {
    return new Fade(flDuration, 1.0f, bStopAtEnd);
}

Source *Source::AllocateHoldAndFadeDownSource(int bStopAtEnd, float flHold, float flFade) {
    return new HoldAndFadeDown(flHold, flFade, bStopAtEnd);
}
