#include "rnd/transanim.h"

#include <algorithm>
#include <list>
#include <string.h>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

// The only revision this build writes, and the highest Load() accepts.
constexpr int kTransAnimRevision = 2;

// Floats of a keyframe vector that reach a stream. The rotation channel writes all four instead.
constexpr int kTransKeyStoredFloatCount = 3;

constexpr char kCountFormat[] = "%d";
constexpr char kSizeFormat[] = "%u";
constexpr char kFloatFormat[] = "%.2f";
constexpr char kQuotedTextFormat[] = "\"%s\"";
constexpr char kTrueText[] = "true";
constexpr char kFalseText[] = "false";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
static const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

// De-inlined from the two places DumpText() repeats it for its two targets. The binary tests both
// the pointer and the virtual-base pointer it converts to, and the second test is what g++ 2.x
// emits for an upcast to a virtual base of a pointer that may be null.
static inline FailSink &DumpTargetName(FailSink &sink, const Object *pTarget) {
    if (pTarget != nullptr) {
        sink.Format(kQuotedTextFormat, NameText(pTarget));
    } else {
        sink.Print("no object");
    }
    return sink;
}

// De-inlined from the two places Save() repeats it. An absent target writes one zero byte, which
// a reader resolves to no object.
static inline Stream &WriteTargetName(Stream &stream, const Object *pTarget) {
    if (pTarget != nullptr) {
        stream.WriteBytes(NameText(pTarget), pTarget->mName.mLen + 1);
    } else {
        const char cEmpty = 0;
        stream.WriteBytes(&cEmpty, sizeof(cEmpty));
    }
    return stream;
}

// 0x004fd348. A mode outside the two below writes nothing rather than a fallback title.
static FailSink &operator<<(FailSink &sink, TransAnim::Interp nInterp) {
    switch (nInterp) {
    case TransAnim::kInterpLinear:
        sink.Print("Linear");
        break;
    case TransAnim::kInterpTCB:
        sink.Print("TCB");
        break;
    }
    return sink;
}

// 0x004f9160
static FailSink &operator<<(FailSink &sink, const TransAnim::RotKey &key) {
    sink.Print("(frame:");
    sink.Format(kFloatFormat, key.mFrame);
    sink.Print(" value:");
    sink.Print("(q:");
    sink.Print("x:");
    sink.Format(kFloatFormat, key.mQuat[0]);
    sink.Print(" y:");
    sink.Format(kFloatFormat, key.mQuat[1]);
    sink.Print(" z:");
    sink.Format(kFloatFormat, key.mQuat[2]);
    sink.Print(" w:");
    sink.Format(kFloatFormat, key.mQuat[3]);
    sink.Print(" t:");
    sink.Format(kFloatFormat, key.mShape[TransAnim::kShapeTension]);
    sink.Print(" c:");
    sink.Format(kFloatFormat, key.mShape[TransAnim::kShapeContinuity]);
    sink.Print(" b:");
    sink.Format(kFloatFormat, key.mShape[TransAnim::kShapeBias]);
    sink.Print(")");
    sink.Print(")");
    return sink;
}

// 0x004f9480
static FailSink &operator<<(FailSink &sink, const TransAnim::TransKey &key) {
    sink.Print("(frame:");
    sink.Format(kFloatFormat, key.mFrame);
    sink.Print(" value:");
    sink.Print("(v:");
    sink.Print("(x:");
    sink.Format(kFloatFormat, key.mValue[0]);
    sink.Print(" y:");
    sink.Format(kFloatFormat, key.mValue[1]);
    sink.Print(" z:");
    sink.Format(kFloatFormat, key.mValue[2]);
    sink.Print(")");
    sink.Print(" t:");
    sink.Format(kFloatFormat, key.mShape[TransAnim::kShapeTension]);
    sink.Print(" c:");
    sink.Format(kFloatFormat, key.mShape[TransAnim::kShapeContinuity]);
    sink.Print(" b:");
    sink.Format(kFloatFormat, key.mShape[TransAnim::kShapeBias]);
    sink.Print(")");
    sink.Print(")");
    return sink;
}

// 0x004f9360
static FailSink &operator<<(FailSink &sink, const std::list<TransAnim::RotKey> &keys) {
    sink.Print("(size:");
    sink.Format(kSizeFormat, keys.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<TransAnim::RotKey>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        sink.Print("\n");
        sink.Format(kCountFormat, nIndex);
        sink.Print("\t");
        sink << *it;
        ++nIndex;
    }
    return sink;
}

// 0x004f9668
static FailSink &operator<<(FailSink &sink, const std::list<TransAnim::TransKey> &keys) {
    sink.Print("(size:");
    sink.Format(kSizeFormat, keys.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<TransAnim::TransKey>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        sink.Print("\n");
        sink.Format(kCountFormat, nIndex);
        sink.Print("\t");
        sink << *it;
        ++nIndex;
    }
    return sink;
}

// 0x004f9788. The padding float of each vector is not written, and the shape triple is written
// ahead of the two tangents rather than after them.
static Stream &operator<<(Stream &stream, const TransAnim::TransKey &key) {
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        float flValue = key.mValue[nAxis];
        stream.Write(&flValue, sizeof(flValue));
    }
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        float flShape = key.mShape[nAxis];
        stream.Write(&flShape, sizeof(flShape));
    }
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        float flIn = key.mTangentIn[nAxis];
        stream.Write(&flIn, sizeof(flIn));
    }
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        float flOut = key.mTangentOut[nAxis];
        stream.Write(&flOut, sizeof(flOut));
    }

    float flFrame = key.mFrame;
    stream.Write(&flFrame, sizeof(flFrame));
    return stream;
}

// 0x004f9a68. The rotation channel writes the fourth float of the quaternion and of both
// tangents, which is the one difference from the vector channels.
static Stream &operator<<(Stream &stream, const TransAnim::RotKey &key) {
    for (int nAxis = 0; nAxis < kXfmRowFloatCount; ++nAxis) {
        float flValue = key.mQuat[nAxis];
        stream.Write(&flValue, sizeof(flValue));
    }
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        float flShape = key.mShape[nAxis];
        stream.Write(&flShape, sizeof(flShape));
    }
    for (int nAxis = 0; nAxis < kXfmRowFloatCount; ++nAxis) {
        float flIn = key.mTangentIn[nAxis];
        stream.Write(&flIn, sizeof(flIn));
    }
    for (int nAxis = 0; nAxis < kXfmRowFloatCount; ++nAxis) {
        float flOut = key.mTangentOut[nAxis];
        stream.Write(&flOut, sizeof(flOut));
    }

    float flFrame = key.mFrame;
    stream.Write(&flFrame, sizeof(flFrame));
    return stream;
}

// 0x004f99b0
static Stream &operator<<(Stream &stream, const std::list<TransAnim::TransKey> &keys) {
    int nCount = keys.size();
    stream.Write(&nCount, sizeof(nCount));

    for (std::list<TransAnim::TransKey>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        stream << *it;
    }
    return stream;
}

// 0x004f9d00
static Stream &operator<<(Stream &stream, const std::list<TransAnim::RotKey> &keys) {
    int nCount = keys.size();
    stream.Write(&nCount, sizeof(nCount));

    for (std::list<TransAnim::RotKey>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        stream << *it;
    }
    return stream;
}

// 0x004f4020
float TransAnim::EndFrame() {
    const float flTrans =
        mFramesOwner->mTransKeys.size() != 0 ? mFramesOwner->mTransKeys.back().mFrame : 0.0f;
    const float flRot =
        mFramesOwner->mRotKeys.size() != 0 ? mFramesOwner->mRotKeys.back().mFrame : 0.0f;
    const float flScale =
        mFramesOwner->mScaleKeys.size() != 0 ? mFramesOwner->mScaleKeys.back().mFrame : 0.0f;
    return std::max(flTrans, std::max(flRot, flScale));
}

// 0x004f4188
float TransAnim::StartFrame() {
    const float flTrans =
        mFramesOwner->mTransKeys.size() != 0 ? mFramesOwner->mTransKeys.front().mFrame : 0.0f;
    const float flRot =
        mFramesOwner->mRotKeys.size() != 0 ? mFramesOwner->mRotKeys.front().mFrame : 0.0f;
    const float flScale =
        mFramesOwner->mScaleKeys.size() != 0 ? mFramesOwner->mScaleKeys.front().mFrame : 0.0f;
    return std::min(flTrans, std::min(flRot, flScale));
}

// 0x004f2ab0
void TransAnim::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    Drawable::DumpText(sink);

    if (sink.mDumpLevel <= 0) {
        return;
    }
    sink.Print("[TransAnim]\n");

    sink.Print("trans:");
    DumpTargetName(sink, mTrans);
    sink.Print(" framesOwner:");
    DumpTargetName(sink, mFramesOwner);
    sink.Print("\n");

    sink.Print("rotKeys:");
    sink << mRotKeys;
    sink.Print("\n");

    sink.Print("transKeys:");
    sink << mTransKeys;
    sink.Print("\n");

    sink.Print("scaleKeys:");
    sink << mScaleKeys;
    sink.Print("\n");

    sink.Print("rotInterp:");
    sink << static_cast<Interp>(mRotInterp);
    sink.Print(" transInterp:");
    sink << static_cast<Interp>(mTransInterp);
    sink.Print(" scaleInterp:");
    sink << static_cast<Interp>(mScaleInterp);
    sink.Print("\n");

    sink.Print("repeatTrans:");
    sink.Print(mRepeatTrans != 0 ? kTrueText : kFalseText);
    sink.Print(" followPath:");
    sink.Print(mFollowPath != 0 ? kTrueText : kFalseText);
    sink.Print("\n");
}

// 0x004f2d50
//
// The three channels are written interleaved with the flags rather than in one block, and the
// order below is the order the reader has to expect.
void TransAnim::Save(Stream &stream) {
    int nRevision = kTransAnimRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    Animatable::Save(stream);
    Drawable::Save(stream);

    WriteTargetName(stream, mTrans);
    WriteTargetName(stream, mFramesOwner);

    stream << mTransKeys;
    stream << mRotKeys;

    int nRotInterp = mRotInterp;
    stream.Write(&nRotInterp, sizeof(nRotInterp));
    int nTransInterp = mTransInterp;
    stream.Write(&nTransInterp, sizeof(nTransInterp));

    const char cRepeatTrans = static_cast<char>(mRepeatTrans);
    stream.WriteBytes(&cRepeatTrans, sizeof(cRepeatTrans));

    stream << mScaleKeys;

    int nScaleInterp = mScaleInterp;
    stream.Write(&nScaleInterp, sizeof(nScaleInterp));

    const char cFollowPath = static_cast<char>(mFollowPath);
    stream.WriteBytes(&cFollowPath, sizeof(cFollowPath));
}

// 0x004f28c8
void TransAnim::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);
    Drawable::Replace(pFrom, pTo);

    if (mTrans == pFrom) {
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (mTrans != nullptr) {
            mTrans = dynamic_cast<Transformable *>(pTo);
        }
        if (mTrans != nullptr) {
            mTrans->AddRef(this);
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
            mFramesOwner = dynamic_cast<TransAnim *>(pTo);
        }
        if (mFramesOwner != nullptr) {
            mFramesOwner->AddRef(this);
        }
        return;
    }

    // The owner is going away, so its timing is taken over rather than dropped.
    mRotKeys = mFramesOwner->mRotKeys;
    mTransKeys = mFramesOwner->mTransKeys;
    mScaleKeys = mFramesOwner->mScaleKeys;
    mFramesOwner = this;
}

// 0x004f3e90
void TransAnim::Copy(const Object *pSource, unsigned nFlags) {
    const TransAnim *pSourceAnim = dynamic_cast<const TransAnim *>(pSource);

    Animatable::Copy(pSource, nFlags);
    Drawable::Copy(pSource, nFlags);

    if (mTrans != nullptr) {
        mTrans->RemoveRef(this);
    }
    if (mFramesOwner != nullptr) {
        mFramesOwner->RemoveRef(this);
    }

    mTrans = pSourceAnim->mTrans;
    mRotInterp = pSourceAnim->mRotInterp;
    mTransInterp = pSourceAnim->mTransInterp;
    mRepeatTrans = pSourceAnim->mRepeatTrans;
    mScaleInterp = pSourceAnim->mScaleInterp;
    mFollowPath = pSourceAnim->mFollowPath;

    if (pSourceAnim->mFramesOwner == pSourceAnim && (nFlags & kCopyShareFrames) == 0) {
        mFramesOwner = this;
        mTransKeys = pSourceAnim->mTransKeys;
        mRotKeys = pSourceAnim->mRotKeys;
        mScaleKeys = pSourceAnim->mScaleKeys;
    } else {
        mFramesOwner = pSourceAnim->mFramesOwner;
        if (mFramesOwner != this) {
            mTransKeys.clear();
            mRotKeys.clear();
            mScaleKeys.clear();
        }
    }

    if (mTrans != nullptr) {
        mTrans->AddRef(this);
    }
    if (mFramesOwner != nullptr) {
        mFramesOwner->AddRef(this);
    }
}

// 0x004fd000
void TransAnim::SetTrans(Transformable *pTrans) {
    if (mTrans != nullptr) {
        mTrans->RemoveRef(this);
    }
    mTrans = pTrans;
    if (pTrans != nullptr) {
        pTrans->AddRef(this);
    }
}

// 0x004fd2c8
void TransAnim::SetFrameSelf(float flFrame) {
    if (mTrans == nullptr) {
        return;
    }

    float aflXfm[kXfmRowCount][kXfmRowFloatCount];
    memcpy(aflXfm, mTrans->mLocalXfm, sizeof(aflXfm));
    EvalFrame(flFrame, aflXfm[0], 0);
    memcpy(mTrans->mLocalXfm, aflXfm, sizeof(aflXfm));
    mTrans->mDirty = 1;
}

} // namespace Rnd
