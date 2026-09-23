#include "rnd/particlesysanim.h"

#include <algorithm>
#include <list>

#include "math/color.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/keychannel.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/particlesys.h"
#include "rnd/stream.h"

namespace Rnd {

namespace {

// The revision from which a record stores mEmitRateRatio, and the one from which Load() would
// retain the channels of an animation that borrows another's frames.
constexpr int kFirstRevisionWithEmitRateRatio = 1;
constexpr int kFirstRevisionKeepingBorrowedKeys = 2;

// Resolve an object name read from a stream to an object of class T, or null.
template <class T>
void ReadTargetName(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(g_manager.Find(name));
}

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

// 0x0052bca0
ParticleSysAnim::ParticleSysAnim(const HxStr &name)
    : Object(name), mParticleSys(nullptr), mFramesOwner(this), mEmitRateRatio(0.0f) {
}

// 0x0052b9c0
ParticleSysAnim::~ParticleSysAnim() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// 0x005267a8
void ParticleSysAnim::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);

    if (mParticleSys == pFrom) {
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mParticleSys != nullptr) {
            mParticleSys = dynamic_cast<ParticleSys *>(pTo);
        }
        if (mParticleSys != nullptr) {
            mParticleSys->AddRef(this);
        }
    }

    if (mFramesOwner != pFrom) {
        return;
    }

    if (pTo != nullptr) {
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mFramesOwner != nullptr) {
            mFramesOwner = dynamic_cast<ParticleSysAnim *>(pTo);
        }
        if (mFramesOwner != nullptr) {
            mFramesOwner->AddRef(this);
        }
        return;
    }

    // The owner is going away, so its channels are taken over rather than dropped.
    mStartColorKeys = mFramesOwner->mStartColorKeys;
    mEndColorKeys = mFramesOwner->mEndColorKeys;
    mEmitRateKeys = mFramesOwner->mEmitRateKeys;
    mFramesOwner = this;
}

// 0x00526ce8
void ParticleSysAnim::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kSerialVersion) {
        g_failSink.Report("Can't load new ParticleSysAnim");
        return;
    }

    Animatable::Load(stream);
    RemoveObjectRefs();
    ReadTargetName(stream, mParticleSys);
    ReadFloatKeys(ReadColorKeys(ReadColorKeys(stream, mStartColorKeys), mEndColorKeys),
                  mEmitRateKeys);
    ReadTargetName(stream, mFramesOwner);
    if (nRevision >= kFirstRevisionWithEmitRateRatio) {
        stream.Read(&mEmitRateRatio, sizeof(mEmitRateRatio));
    }
    if (nRevision < kFirstRevisionKeepingBorrowedKeys) {
        ClearKeys(); // Yes, every revision Load() accepts passes this test.
    }
    AddObjectRefs();
}

// 0x0052c700
void ParticleSysAnim::SetParticleSys(ParticleSys *pParticleSys) {
    if (mParticleSys != nullptr) {
        mParticleSys->RemoveRef(this);
    }
    mParticleSys = pParticleSys;
    if (pParticleSys != nullptr) {
        pParticleSys->AddRef(this);
    }
}

// 0x0052c758
void ParticleSysAnim::ClearKeys() {
    if (mFramesOwner == this) {
        return;
    }
    mStartColorKeys.clear();
    mEndColorKeys.clear();
    mEmitRateKeys.clear();
}

// 0x0052c7a0
void ParticleSysAnim::SetFramesOwner(ParticleSysAnim *pOwner) {
    if (mFramesOwner != nullptr) {
        mFramesOwner->RemoveRef(this);
    }
    mFramesOwner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
    ClearKeys();
}

// 0x0052c818
void ParticleSysAnim::RemoveObjectRefs() {
    if (mParticleSys != nullptr) {
        mParticleSys->RemoveRef(this);
    }
    if (mFramesOwner != nullptr) {
        mFramesOwner->RemoveRef(this);
    }
}

// 0x0052c868
void ParticleSysAnim::AddObjectRefs() {
    if (mParticleSys != nullptr) {
        mParticleSys->AddRef(this);
    }
    if (mFramesOwner != nullptr) {
        mFramesOwner->AddRef(this);
    }
}

// 0x0052c180
Object *CreateRegisteredParticleSysAnim(const HxStr &name) {
    return NewParticleSysAnim(name);
}

} // namespace Rnd
