#include "rnd/particlesysanim.h"

#include <algorithm>
#include <list>

#include "math/color.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"
#include "rnd/object.h"
#include "rnd/particlesys.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// The text dump writes an absent object reference as this literal, and a present one as its
// quoted name. src/rnd/mesh.cpp declares the same pair file-locally for the same reason.
constexpr char kNoObject[] = "no object";

// The name of an object with no name of its own reads as the empty string.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void PrintObjectRef(FailSink &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format("\"%s\"", NameText(pObject));
}

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
}

// Move the low end of a spawn colour range to the interpolated colour of a channel, and shift the
// high end by the same distance so that the spread of the range survives. An empty channel moves
// neither end. SetFrameSelf() inlines the interpolation twice and calls the two colour helpers
// each time.
void BlendColorRange(const std::list<ColorKey> &keys, float flFrame, Color &low, Color &high) {
    if (keys.empty()) {
        return;
    }

    const ColorKey *pFrom = nullptr;
    const ColorKey *pTo = nullptr;
    float flBlend = 0.0f;
    SelectKeyPair(keys, flFrame, pFrom, pTo, flBlend);

    const float flInverse = 1.0f - flBlend;
    Color blended;
    blended.r = pTo->mValue.r * flBlend + pFrom->mValue.r * flInverse;
    blended.g = pTo->mValue.g * flBlend + pFrom->mValue.g * flInverse;
    blended.b = pTo->mValue.b * flBlend + pFrom->mValue.b * flInverse;
    blended.a = pTo->mValue.a * flBlend + pFrom->mValue.a * flInverse;

    Color shifted;
    AddColor(blended, high, shifted);
    SubColor(shifted, low, shifted);
    low = blended;
    high = shifted;
}

} // namespace

// 0x0071af00
HxStr g_particleSysAnimClassName("ParticleSysAnim");

// 0x0052bc90
const HxStr &ParticleSysAnim::ClassName() const {
    return g_particleSysAnimClassName;
}

// 0x00526980
void ParticleSysAnim::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[ParticleSysAnim]\n");
    sink.Print("particleSys:");
    PrintObjectRef(sink, mParticleSys);
    sink.Print(" framesOwner:");
    PrintObjectRef(sink, mFramesOwner);
    sink.Print(" emitRateRatio:");
    sink.Format("%.2f", mEmitRateRatio);
    sink.Print("\n");
    sink.Print("startColorKeys:");
    DumpColorKeys(sink, mStartColorKeys);
    sink.Print("\n");
    sink.Print("endColorKeys:");
    DumpColorKeys(sink, mEndColorKeys);
    sink.Print("\n");
    sink.Print("emitRateKeys:");
    DumpFloatKeys(sink, mEmitRateKeys);
    sink.Print("\n");
}

// 0x00527128
float ParticleSysAnim::EndFrame() {
    const float flStartColor = ChannelEndFrame(mFramesOwner->mStartColorKeys);
    const float flEndColor = ChannelEndFrame(mFramesOwner->mEndColorKeys);
    const float flEmitRate = ChannelEndFrame(mFramesOwner->mEmitRateKeys);
    return std::max(flStartColor, std::max(flEndColor, flEmitRate));
}

// 0x00527290
void ParticleSysAnim::SetFrameSelf(float flFrame) {
    if (mParticleSys == nullptr) {
        return;
    }

    BlendColorRange(mFramesOwner->mStartColorKeys,
                    flFrame,
                    mParticleSys->mStartColorLow,
                    mParticleSys->mStartColorHigh);
    BlendColorRange(mFramesOwner->mEndColorKeys,
                    flFrame,
                    mParticleSys->mEndColorLow,
                    mParticleSys->mEndColorHigh);

    if (mFramesOwner->mEmitRateKeys.empty()) {
        return;
    }

    const FloatKey *pFrom = nullptr;
    const FloatKey *pTo = nullptr;
    float flBlend = 0.0f;
    SelectKeyPair(mFramesOwner->mEmitRateKeys, flFrame, pFrom, pTo, flBlend);
    const float flRate = (pTo->mValue - pFrom->mValue) * flBlend + pFrom->mValue;

    // A zero low rate retains the previous ratio rather than dividing by zero.
    if (mParticleSys->mEmitRateLow != 0.0f) {
        mEmitRateRatio = mParticleSys->mEmitRateHigh / mParticleSys->mEmitRateLow;
    }
    mParticleSys->mEmitRateLow = flRate;
    mParticleSys->mEmitRateHigh = flRate * mEmitRateRatio;
}

// 0x00526b68
void ParticleSysAnim::Save(Stream &stream) {
    const int nVersion = kSerialVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    Animatable::Save(stream);

    WriteObjectRef(stream, mParticleSys);
    WriteColorKeys(stream, mStartColorKeys);
    WriteColorKeys(stream, mEndColorKeys);
    WriteFloatKeys(stream, mEmitRateKeys);
    WriteObjectRef(stream, mFramesOwner);
    stream.Write(&mEmitRateRatio, sizeof(mEmitRateRatio));
}

// 0x00526fc8
void ParticleSysAnim::Copy(const Object *pSource, unsigned nFlags) {
    const ParticleSysAnim *pSourceAnim = dynamic_cast<const ParticleSysAnim *>(pSource);

    Animatable::Copy(pSource, nFlags);

    if (mParticleSys != nullptr) {
        mParticleSys->RemoveRef(this);
    }
    if (mFramesOwner != nullptr) {
        mFramesOwner->RemoveRef(this);
    }

    mParticleSys = pSourceAnim->mParticleSys;
    mEmitRateRatio = pSourceAnim->mEmitRateRatio;
    if ((nFlags & kCopyShareKeys) != 0 || pSourceAnim->mFramesOwner != pSourceAnim) {
        mFramesOwner = pSourceAnim->mFramesOwner;
        if (mFramesOwner != this) {
            mStartColorKeys.clear();
            mEndColorKeys.clear();
            mEmitRateKeys.clear();
        }
    } else {
        mFramesOwner = this;
        mStartColorKeys = pSourceAnim->mStartColorKeys;
        mEndColorKeys = pSourceAnim->mEndColorKeys;
        mEmitRateKeys = pSourceAnim->mEmitRateKeys;
    }

    if (mParticleSys != nullptr) {
        mParticleSys->AddRef(this);
    }
    if (mFramesOwner != nullptr) {
        mFramesOwner->AddRef(this);
    }
}

// 0x0052c180
ParticleSysAnim *NewParticleSysAnim(const HxStr &name) {
    // The allocation is untagged here, unlike every other renderer class, and it is exactly 0x4c
    // bytes rather than a rounded size.
    return new ParticleSysAnim(name);
}

} // namespace Rnd
