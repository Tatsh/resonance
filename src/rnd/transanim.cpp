#include "rnd/transanim.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <list>
#include <string.h>

#include "math/quaternion.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/keychannel.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"

namespace Rnd {

// The only revision this build writes, and the highest Load() accepts.
constexpr int kTransAnimRevision = 2;

// The revision from which a record stores the scale channel, and the one from which the rotation
// and translation keys arrive in their current form along with the follow-path flag.
constexpr int kScaleChannelRevision = 1;
constexpr int kCurrentKeyRevision = 2;

// Floats of a keyframe vector that reach a stream. The rotation channel writes all four instead.
constexpr int kTransKeyStoredFloatCount = 3;

// Index of the padding float that ends each vector of a keyframe.
constexpr int kPaddingFloat = 3;

// The weight a tangent at the end of a channel gives the one chord it has.
constexpr float kEndTangentChordWeight = 1.5f;

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

// De-inlined from the three quaternion writes of the rotation keyframe writer, which emits the
// four components in order.
static inline void WriteQuat(Stream &stream, const Quat &quat) {
    float flComponent = quat.x;
    stream.Write(&flComponent, sizeof(flComponent));
    flComponent = quat.y;
    stream.Write(&flComponent, sizeof(flComponent));
    flComponent = quat.z;
    stream.Write(&flComponent, sizeof(flComponent));
    flComponent = quat.w;
    stream.Write(&flComponent, sizeof(flComponent));
}

// Divisor and half weight of the Kochanek-Bartels tangent terms.
constexpr float kTangentThird = 3.0f;
constexpr float kTangentHalf = 0.5f;

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

// Resolve a name read from the stream to an object of type T, or null.
template <class T>
static void ReadTargetName(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(g_manager.Find(name));
}

// A rotation keyframe as a record below revision 2 stores it, a quaternion and a frame.
struct LegacyRotKey {
    Quat mValue;
    float mFrame;
};

// 0x004f9f80
static Stream &operator>>(Stream &stream, LegacyRotKey &key) {
    stream.Read(&key.mValue.x, sizeof(float));
    stream.Read(&key.mValue.y, sizeof(float));
    stream.Read(&key.mValue.z, sizeof(float));
    stream.Read(&key.mValue.w, sizeof(float));
    stream.Read(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// 0x004fd6b8
static Stream &operator>>(Stream &stream, std::list<LegacyRotKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        stream >> key;
    }
    return stream;
}

// 0x004fa500
// The same order the writer uses, with the padding float of each vector left as it was.
static Stream &operator>>(Stream &stream, TransAnim::TransKey &key) {
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        stream.Read(&key.mValue[nAxis], sizeof(float));
    }
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        stream.Read(&key.mShape[nAxis], sizeof(float));
    }
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        stream.Read(&key.mTangentIn[nAxis], sizeof(float));
    }
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        stream.Read(&key.mTangentOut[nAxis], sizeof(float));
    }
    stream.Read(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// De-inlined from the four quaternion reads of the rotation keyframe reader.
static inline void ReadQuat(Stream &stream, Quat &quat) {
    stream.Read(&quat.x, sizeof(float));
    stream.Read(&quat.y, sizeof(float));
    stream.Read(&quat.z, sizeof(float));
    stream.Read(&quat.w, sizeof(float));
}

// 0x004fa948
static Stream &operator>>(Stream &stream, TransAnim::RotKey &key) {
    ReadQuat(stream, key.mQuat);
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        stream.Read(&key.mShape[nAxis], sizeof(float));
    }
    ReadQuat(stream, key.mTangentIn);
    ReadQuat(stream, key.mTangentOut);
    stream.Read(&key.mFrame, sizeof(key.mFrame));
    return stream;
}

// 0x004fa6a8
// Every new element starts from a keyframe whose three vectors have a padding float of 1.0.
static Stream &operator>>(Stream &stream, std::list<TransAnim::TransKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    TransAnim::TransKey defaultKey = {};
    defaultKey.mValue[kPaddingFloat] = 1.0f;
    defaultKey.mTangentIn[kPaddingFloat] = 1.0f;
    defaultKey.mTangentOut[kPaddingFloat] = 1.0f;
    keys.resize(nCount, defaultKey);
    for (auto &key : keys) {
        stream >> key;
    }
    return stream;
}

// 0x004fab40
static Stream &operator>>(Stream &stream, std::list<TransAnim::RotKey> &keys) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    keys.resize(nCount);
    for (auto &key : keys) {
        stream >> key;
    }
    return stream;
}

// Rebuild the tangents of a vector channel after a key is added. A channel of two keys takes the
// chord between them, weighted by each key's tension, and a longer channel rebuilds every key from
// its neighbours, the inner keys first. Load() inlines the body for both vector channels.
static void RebuildTransTangents(std::list<TransAnim::TransKey> &keys) {
    if (keys.size() == 1) {
        return;
    }
    if (keys.size() == 2) {
        TransAnim::TransKey &first = keys.front();
        TransAnim::TransKey &last = keys.back();
        float afChord[kXfmRowFloatCount];
        Vec3Sub(last.mValue, first.mValue, afChord);
        Vec3Scale(afChord, 1.0f - first.mShape[TransAnim::kShapeTension], first.mTangentOut);
        first.mTangentOut[kPaddingFloat] = 1.0f;
        Vec3Sub(last.mValue, first.mValue, afChord);
        Vec3Scale(afChord, 1.0f - last.mShape[TransAnim::kShapeTension], last.mTangentIn);
        last.mTangentIn[kPaddingFloat] = 1.0f;
        return;
    }
    for (auto it = std::next(keys.begin()); it != std::prev(keys.end()); ++it) {
        it->ComputeSplineTangents(&*std::prev(it), &*std::next(it));
    }
    keys.front().ComputeSplineTangents(nullptr, &*std::next(keys.begin()));
    keys.back().ComputeSplineTangents(&*std::prev(std::prev(keys.end())), nullptr);
}

// The rotation counterpart of RebuildTransTangents(). A channel of two keys gives each key the
// tangent it would take with the other as its one neighbour, which the binary writes out inline.
static void RebuildRotTangents(std::list<TransAnim::RotKey> &keys) {
    if (keys.size() == 1) {
        return;
    }
    if (keys.size() == 2) {
        keys.front().ComputeSplineTangents(nullptr, &keys.back());
        keys.back().ComputeSplineTangents(&keys.front(), nullptr);
        return;
    }
    for (auto it = std::next(keys.begin()); it != std::prev(keys.end()); ++it) {
        it->ComputeSplineTangents(&*std::prev(it), &*std::next(it));
    }
    keys.front().ComputeSplineTangents(nullptr, &*std::next(keys.begin()));
    keys.back().ComputeSplineTangents(&*std::prev(std::prev(keys.end())), nullptr);
}

// Add a key read in the form below revision 2 to a vector channel. The tangents take only their
// padding floats, which the binary leaves as stack contents apart from that word, and the shape is
// zeroed.
static void AppendLegacyTransKey(std::list<TransAnim::TransKey> &keys, const Vector3Key &legacy) {
    TransAnim::TransKey key = {};
    key.mValue[0] = legacy.mValue.x;
    key.mValue[1] = legacy.mValue.y;
    key.mValue[2] = legacy.mValue.z;
    key.mValue[kPaddingFloat] = legacy.mValue.w;
    key.mTangentIn[kPaddingFloat] = 1.0f;
    key.mTangentOut[kPaddingFloat] = 1.0f;
    key.mFrame = legacy.mFrame;
    keys.push_back(key);
    keys.sort(); // Yes, the binary sorts the channel again after every key.
    RebuildTransTangents(keys);
}

// The rotation counterpart of AppendLegacyTransKey(). The tangents are stack contents in the binary
// and are zeroed here.
static void AppendLegacyRotKey(std::list<TransAnim::RotKey> &keys, const LegacyRotKey &legacy) {
    TransAnim::RotKey key = {};
    key.mQuat = legacy.mValue;
    key.mFrame = legacy.mFrame;
    keys.push_back(key);
    keys.sort(); // Yes, the binary sorts the channel again after every key.
    RebuildRotTangents(keys);
}

// 0x004fd348
// A mode outside the two below writes nothing rather than a fallback title.
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
    sink.Format(kFloatFormat, key.mQuat.x);
    sink.Print(" y:");
    sink.Format(kFloatFormat, key.mQuat.y);
    sink.Print(" z:");
    sink.Format(kFloatFormat, key.mQuat.z);
    sink.Print(" w:");
    sink.Format(kFloatFormat, key.mQuat.w);
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

// 0x004f9788
// The padding float of each vector is not written, and the shape triple is written ahead of the
// two tangents rather than after them.
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

// 0x004f9a68
// Unlike the vector channels, the rotation channel writes the fourth float of the quaternion and
// of both tangents.
static Stream &operator<<(Stream &stream, const TransAnim::RotKey &key) {
    WriteQuat(stream, key.mQuat);
    for (int nAxis = 0; nAxis < kTransKeyStoredFloatCount; ++nAxis) {
        float flShape = key.mShape[nAxis];
        stream.Write(&flShape, sizeof(flShape));
    }
    WriteQuat(stream, key.mTangentIn);
    WriteQuat(stream, key.mTangentOut);

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

// Rows of the transform EvalFrame() writes.
enum XfmRow { kXfmRowBasisX = 0, kXfmRowBasisY = 1, kXfmRowBasisZ = 2, kXfmRowTranslation = 3 };

// The fewest translation keys a follow path orients along.
constexpr std::size_t kMinFollowPathKeys = 2;

// VU0's vf0, the translation row an empty channel is reset to.
constexpr float kIdentityTranslation[] = {0.0f, 0.0f, 0.0f, 1.0f};

static inline float *XfmRowOf(float *pXfm, int nRow) {
    return &pXfm[nRow * kXfmRowFloatCount];
}

// Write the identity into the three basis rows. The padding floats are not written.
static inline void SetIdentityBasis(float *pXfm) {
    for (int nRow = kXfmRowBasisX; nRow <= kXfmRowBasisZ; ++nRow) {
        float *const pRow = XfmRowOf(pXfm, nRow);
        for (int i = 0; i < kPaddingFloat; ++i) {
            pRow[i] = (i == nRow) ? 1.0f : 0.0f;
        }
    }
}

// The linear blend EvalFrame() runs on VU0. The padding float comes from pTo.
static inline void LerpKeyVector(const float *pFrom, const float *pTo, float flT, float *pOut) {
    for (int i = 0; i < kPaddingFloat; ++i) {
        pOut[i] = pTo[i] * flT + pFrom[i] * (1.0f - flT);
    }
    pOut[kPaddingFloat] = pTo[kPaddingFloat];
}

// Find the keys of a channel on either side of flFrame and the parameter between them. A frame at
// or before the first key, or at or after the last, yields that key twice. An empty channel writes
// nothing. EvalFrame() inlines this once per channel.
template <class Key>
static inline void FindBracketingKeys(
    const std::list<Key> &keys, float flFrame, const Key *&pPrev, const Key *&pNext, float &flT) {
    if (keys.empty()) {
        return;
    }
    if (flFrame <= keys.front().mFrame) {
        pPrev = &keys.front();
        pNext = pPrev;
        flT = 0.0f;
        return;
    }
    if (keys.back().mFrame <= flFrame) {
        pPrev = &keys.back();
        pNext = pPrev;
        flT = 1.0f;
        return;
    }
    auto prev = keys.begin();
    for (auto it = std::next(prev); it != keys.end(); ++it) {
        if (flFrame <= it->mFrame) {
            pPrev = &*prev;
            pNext = &*it;
            flT = (flFrame - prev->mFrame) / (it->mFrame - prev->mFrame);
            return;
        }
        prev = it;
    }
}

// Orient the basis rows along the translation curve at flFrame, against a +z reference. Returns
// false where EvalFrame() writes the identity instead.
static inline bool BuildFollowPathBasis(const std::list<TransAnim::TransKey> &path,
                                        int nInterp,
                                        float flFrame,
                                        float *pXfm) {
    if (path.size() < kMinFollowPathKeys) {
        return false;
    }
    const TransAnim::TransKey *pPrev = nullptr;
    const TransAnim::TransKey *pNext = nullptr;
    float flT = 0.0f;
    FindBracketingKeys(path, flFrame, pPrev, pNext, flT);

    const Vector3 reference{0.0f, 0.0f, 1.0f, 1.0f};
    if (nInterp == TransAnim::kInterpTCB) {
        const Vector3 direction = pPrev->EvaluateSplineDerivative(pNext, flT);
        Mat33BuildOrthonormal(&direction.x, &reference.x, pXfm);
        return true;
    }
    if (pPrev == pNext) {
        return false;
    }
    Vector3 direction;
    direction.w = 1.0f;
    Vec3Sub(pNext->mValue, pPrev->mValue, &direction.x);
    Mat33BuildOrthonormal(&direction.x, &reference.x, pXfm);
    return true;
}

// 0x004f42f0
void TransAnim::EvalFrame(float flFrame, float *pXfm, int nResetEmpty) {
    float *const pTranslation = XfmRowOf(pXfm, kXfmRowTranslation);
    if (!mFramesOwner->mTransKeys.empty()) {
        std::list<TransKey> &keys = mFramesOwner->mTransKeys;
        Vector3 offset;
        offset.w = 1.0f;
        if (mRepeatTrans) {
            if (flFrame < 0.0f && mTransInterp == kInterpTCB) {
                const TransKey &first = keys.front();
                // Yes, a channel of one key divides by the frame of the list head here.
                const TransKey &second = *std::next(keys.begin());
                const float flScale = flFrame / (second.mFrame - first.mFrame);
                flFrame = 0.0f;
                offset.x = first.mTangentOut[0] * flScale;
                offset.y = first.mTangentOut[1] * flScale;
                offset.z = first.mTangentOut[2] * flScale;
            } else {
                const TransKey &first = keys.front();
                const TransKey &last = keys.back();
                flFrame -= first.mFrame;
                const float flSpan = last.mFrame - first.mFrame;
                const float flWraps =
                    static_cast<float>(static_cast<int>(std::floor(flFrame / flSpan)));
                flFrame -= flWraps * flSpan;
                float afChord[kXfmRowFloatCount];
                afChord[kPaddingFloat] = 1.0f;
                Vec3Sub(last.mValue, first.mValue, afChord);
                offset.x = afChord[0] * flWraps;
                offset.y = afChord[1] * flWraps;
                offset.z = afChord[2] * flWraps;
            }
            if (mTransInterp == kInterpTCB) {
                std::copy(std::begin(keys.front().mTangentOut),
                          std::end(keys.front().mTangentOut),
                          keys.back().mTangentIn);
            }
        }

        const TransKey *pPrev = nullptr;
        const TransKey *pNext = nullptr;
        float flT = 0.0f;
        FindBracketingKeys<TransKey>(keys, flFrame, pPrev, pNext, flT);
        if (mTransInterp == kInterpTCB) {
            pPrev->EvaluateSpline(pNext, pTranslation, flT);
        } else {
            LerpKeyVector(pPrev->mValue, pNext->mValue, flT, pTranslation);
        }
        if (mRepeatTrans) {
            AddVec3(pTranslation, &offset.x, pTranslation);
        }
    } else if (nResetEmpty) {
        std::copy(std::begin(kIdentityTranslation), std::end(kIdentityTranslation), pTranslation);
    }

    if (mFollowPath) {
        if (!BuildFollowPathBasis(mFramesOwner->mTransKeys, mTransInterp, flFrame, pXfm)) {
            SetIdentityBasis(pXfm);
        }
    } else if (!mFramesOwner->mRotKeys.empty()) {
        const RotKey *pPrev = nullptr;
        const RotKey *pNext = nullptr;
        float flT = 0.0f;
        FindBracketingKeys<RotKey>(mFramesOwner->mRotKeys, flFrame, pPrev, pNext, flT);
        Quat rotation;
        if (mRotInterp == kInterpTCB) {
            pPrev->EvaluateSpline(pNext, rotation, flT);
        } else {
            QuatSlerp(pPrev->mQuat, pNext->mQuat, rotation, flT);
        }
        QuatToMat33(rotation, pXfm);
    } else if (nResetEmpty) {
        SetIdentityBasis(pXfm);
    }

    const std::list<TransKey> &scaleKeys = mFramesOwner->mScaleKeys;
    if (scaleKeys.empty()) {
        return;
    }
    Vector3 scale;
    scale.w = 1.0f;
    const TransKey *pPrev = nullptr;
    const TransKey *pNext = nullptr;
    float flT = 0.0f;
    FindBracketingKeys(scaleKeys, flFrame, pPrev, pNext, flT);
    if (mScaleInterp == kInterpTCB) {
        pPrev->EvaluateSpline(pNext, &scale.x, flT);
    } else {
        LerpKeyVector(pPrev->mValue, pNext->mValue, flT, &scale.x);
    }
    float *const pBasisX = XfmRowOf(pXfm, kXfmRowBasisX);
    float *const pBasisY = XfmRowOf(pXfm, kXfmRowBasisY);
    float *const pBasisZ = XfmRowOf(pXfm, kXfmRowBasisZ);
    Vec3Scale(pBasisX, scale.x, pBasisX);
    Vec3Scale(pBasisY, scale.y, pBasisY);
    Vec3Scale(pBasisZ, scale.z, pBasisZ);
}

// 0x00552588
void TransAnim::RotKey::ComputeSplineTangents(const RotKey *pPrev, const RotKey *pNext) {
    const float flTension = mShape[kShapeTension];
    const float flContinuity = mShape[kShapeContinuity];
    const float flBias = mShape[kShapeBias];
    if (pPrev != nullptr && pNext != nullptr) {
        Quat toPrev;
        Quat toNext;
        Quat mid;
        QuatSlerp(mQuat, pPrev->mQuat, toPrev, -(flBias + 1.0f) / kTangentThird);
        QuatSlerp(mQuat, pNext->mQuat, toNext, (1.0f - flBias) / kTangentThird);
        QuatSlerp(toPrev, toNext, mid, kTangentHalf - flContinuity * kTangentHalf);
        QuatSlerp(mQuat, mid, mTangentOut, -(flTension - 1.0f));
        QuatSlerp(toPrev, toNext, mid, flContinuity * kTangentHalf + kTangentHalf);
        QuatSlerp(mQuat, mid, mTangentIn, flTension - 1.0f);
    } else if (pNext != nullptr) {
        QuatSlerp(mQuat,
                  pNext->mQuat,
                  mTangentOut,
                  (1.0f - flTension) * (flContinuity * flBias + 1.0f) / kTangentThird);
    } else if (pPrev != nullptr) {
        QuatSlerp(mQuat,
                  pPrev->mQuat,
                  mTangentIn,
                  (1.0f - flTension) * (1.0f - flContinuity * flBias) / kTangentThird);
    }
}

// 0x00552748
void TransAnim::TransKey::ComputeSplineTangents(const TransKey *pPrev, const TransKey *pNext) {
    const float flTension = mShape[kShapeTension];
    const float flContinuity = mShape[kShapeContinuity];
    const float flBias = mShape[kShapeBias];
    float afTerm[kXfmRowFloatCount];
    float afWeighted[kXfmRowFloatCount];
    if (pPrev != nullptr && pNext != nullptr) {
        float afToPrev[kXfmRowFloatCount];
        float afToNext[kXfmRowFloatCount];
        Vec3Sub(mValue, pPrev->mValue, afToPrev);
        Vec3Scale(afToPrev, flBias + 1.0f, afToPrev);
        Vec3Sub(pNext->mValue, mValue, afToNext);
        Vec3Scale(afToNext, 1.0f - flBias, afToNext);

        Vec3Sub(afToNext, afToPrev, afTerm);
        Vec3Scale(afTerm, kTangentHalf - flContinuity * kTangentHalf, afTerm);
        AddVec3(afToPrev, afTerm, afWeighted);
        Vec3Scale(afWeighted, 1.0f - flTension, mTangentOut);
        mTangentOut[kPaddingFloat] = 1.0f;

        Vec3Sub(afToNext, afToPrev, afTerm);
        Vec3Scale(afTerm, flContinuity * kTangentHalf + kTangentHalf, afTerm);
        AddVec3(afToPrev, afTerm, afWeighted);
        Vec3Scale(afWeighted, 1.0f - flTension, mTangentIn);
        mTangentIn[kPaddingFloat] = 1.0f;
    } else if (pNext != nullptr) {
        Vec3Sub(pNext->mValue, mValue, afTerm);
        Vec3Scale(afTerm, kEndTangentChordWeight, afTerm);
        Vec3Scale(pNext->mTangentIn, kTangentHalf, afWeighted);
        Vec3Scale(afWeighted, flBias + 1.0f, afWeighted);
        Vec3Sub(afTerm, afWeighted, afWeighted);
        Vec3Scale(afWeighted, 1.0f - flTension, mTangentOut);
        mTangentOut[kPaddingFloat] = 1.0f;
    } else if (pPrev != nullptr) {
        Vec3Sub(mValue, pPrev->mValue, afTerm);
        Vec3Scale(afTerm, kEndTangentChordWeight, afTerm);
        Vec3Scale(pPrev->mTangentOut, kTangentHalf, afWeighted);
        Vec3Scale(afWeighted, flBias + 1.0f, afWeighted);
        Vec3Sub(afTerm, afWeighted, afWeighted);
        Vec3Scale(afWeighted, 1.0f - flTension, mTangentIn);
        mTangentIn[kPaddingFloat] = 1.0f;
    }
}

// The coefficients below are those of the cubic Hermite basis and of its derivative.

// 0x00552af8
void TransAnim::TransKey::EvaluateSpline(const TransKey *pNext, float *pOut, float flT) const {
    if (flT == 0.0f) {
        std::copy(std::begin(mValue), std::end(mValue), pOut);
        return;
    }
    if (flT == 1.0f) {
        std::copy(std::begin(pNext->mValue), std::end(pNext->mValue), pOut);
        return;
    }
    const float flT2 = flT * flT;
    const float flT3 = flT2 * flT;
    const float flThreeT2 = flT2 * 3.0f;
    float afSum[kXfmRowFloatCount];
    float afTerm[kXfmRowFloatCount];
    afSum[kPaddingFloat] = 1.0f;
    afTerm[kPaddingFloat] = 1.0f;
    Vec3Scale(mValue, flT3 + flT3 - flThreeT2 + 1.0f, afSum);
    Vec3Scale(mTangentOut, flT3 - (flT2 + flT2) + flT, afTerm);
    AddVec3(afSum, afTerm, afSum);
    Vec3Scale(pNext->mValue, flT3 * -2.0f + flThreeT2, afTerm);
    AddVec3(afSum, afTerm, afSum);
    Vec3Scale(pNext->mTangentIn, flT3 - flT2, afTerm);
    AddVec3(afSum, afTerm, afSum);
    std::copy(std::begin(afSum), std::end(afSum), pOut);
}

// 0x00552cb8
Vector3 TransAnim::TransKey::EvaluateSplineDerivative(const TransKey *pNext, float flT) const {
    const float flT2 = flT * flT;
    const float flSixT = flT * 6.0f;
    const float flThreeT2 = flT2 * 3.0f;
    Vector3 sum;
    Vector3 term;
    sum.w = 1.0f;
    term.w = 1.0f;
    Vec3Scale(mValue, flT2 * 6.0f - flSixT, &sum.x);
    Vec3Scale(mTangentOut, flThreeT2 - flT * 4.0f + 1.0f, &term.x);
    AddVec3(&sum.x, &term.x, &sum.x);
    Vec3Scale(pNext->mValue, flT2 * -6.0f + flSixT, &term.x);
    AddVec3(&sum.x, &term.x, &sum.x);
    Vec3Scale(pNext->mTangentIn, flThreeT2 - (flT + flT), &term.x);
    AddVec3(&sum.x, &term.x, &sum.x);
    return sum;
}

// Parameter step of the sum in SplineLength(), and the weight of each sample.
constexpr float kSplineLengthStep = 0.005f;

// 0x00554d90
float TransAnim::TransKey::SplineLength(const TransKey *pNext) const {
    float flLength = 0.0f;
    float flT = 0.0f;
    do {
        const Vector3 derivative = EvaluateSplineDerivative(pNext, flT);
        flT += kSplineLengthStep;
        flLength += std::sqrt(derivative.x * derivative.x + derivative.y * derivative.y +
                              derivative.z * derivative.z) *
                    kSplineLengthStep;
    } while (flT < 1.0f);
    return flLength;
}

// 0x00554c68
void TransAnim::RotKey::EvaluateSpline(const RotKey *pNext, Quat &out, float flT) const {
    if (flT == 0.0f) {
        out = mQuat;
        return;
    }
    if (flT == 1.0f) {
        out = pNext->mQuat;
        return;
    }
    Quat outgoing;
    Quat middle;
    Quat incoming;
    QuatSlerp(mQuat, mTangentOut, outgoing, flT);
    QuatSlerp(mTangentOut, pNext->mTangentIn, middle, flT);
    QuatSlerp(pNext->mTangentIn, pNext->mQuat, incoming, flT);
    Quat first;
    Quat second;
    QuatSlerp(outgoing, middle, first, flT);
    QuatSlerp(middle, incoming, second, flT);
    QuatSlerp(first, second, out, flT);
}

// 0x004fc000
TransAnim::TransAnim(const HxStr &name)
    : Object(name), mTrans(nullptr), mRotInterp(kInterpLinear), mTransInterp(kInterpTCB),
      mScaleInterp(kInterpLinear), mFramesOwner(this), mRepeatTrans(0), mFollowPath(0) {
}

// 0x004fbb78
TransAnim::~TransAnim() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// 0x004fbf60
const HxStr &TransAnim::ClassName() const {
    return g_transAnimClassName;
}

// 0x004fd058
void TransAnim::ClearKeys() {
    if (mFramesOwner == this) {
        return;
    }
    mTransKeys.clear();
    mRotKeys.clear();
    mScaleKeys.clear();
}

// 0x004fd168
void TransAnim::AddObjectRefs() {
    if (mTrans != nullptr) {
        mTrans->AddRef(this);
    }
    if (mFramesOwner != nullptr) {
        mFramesOwner->AddRef(this);
    }
}

// 0x004fd118
void TransAnim::RemoveObjectRefs() {
    if (mTrans != nullptr) {
        mTrans->RemoveRef(this);
    }
    if (mFramesOwner != nullptr) {
        mFramesOwner->RemoveRef(this);
    }
}

// 0x004fd0a0
void TransAnim::SetFramesOwner(TransAnim *pOwner) {
    if (mFramesOwner != nullptr) {
        mFramesOwner->RemoveRef(this);
    }
    mFramesOwner = pOwner;
    if (pOwner != nullptr) {
        pOwner->AddRef(this);
    }
    ClearKeys();
}

// 0x00706828
HxStr g_transAnimClassName("TransAnim");

// 0x004fc740
TransAnim *NewTransAnim(const HxStr &name) {
    return new TransAnim(name);
}

// 0x00706820
TransAnim *(*g_pfnNewTransAnim)(const HxStr &name) = NewTransAnim;

// 0x004fba90
TransAnim *NewTransAnimThroughHook(const HxStr &name) {
    return g_pfnNewTransAnim(name);
}

// 0x004fbf70
Object *CreateRegisteredTransAnim(const HxStr &name) {
    return g_pfnNewTransAnim(name);
}

// 0x004fba50
void RegisterTransAnimClass() {
    g_pfnNewTransAnim = NewTransAnim;
    g_manager.RegisterClass(g_transAnimClassName, CreateRegisteredTransAnim);
}

// 0x004f2f68
void TransAnim::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kTransAnimRevision) {
        g_failSink.Report("Can't load new TransAnim\n");
        return;
    }

    Animatable::Load(stream);
    Drawable::Load(stream);
    RemoveObjectRefs();
    ReadTargetName(stream, mTrans);

    std::list<LegacyRotKey> legacyRotKeys;
    std::list<Vector3Key> legacyTransKeys;
    std::list<Vector3Key> legacyScaleKeys;
    if (nRevision < kCurrentKeyRevision) {
        stream >> legacyRotKeys;
        ReadVector3Keys(stream, legacyTransKeys);
    }

    ReadTargetName(stream, mFramesOwner);
    stream >> mTransKeys;
    stream >> mRotKeys;
    stream.Read(&mRotInterp, sizeof(mRotInterp));
    stream.Read(&mTransInterp, sizeof(mTransInterp));
    char chRepeatTrans = '\0';
    stream.ReadBytes(&chRepeatTrans, sizeof(chRepeatTrans));
    mRepeatTrans = chRepeatTrans != '\0';

    if (nRevision >= kScaleChannelRevision) {
        if (nRevision < kCurrentKeyRevision) {
            ReadVector3Keys(stream, legacyScaleKeys);
        }
        stream >> mScaleKeys;
        stream.Read(&mScaleInterp, sizeof(mScaleInterp));
    }

    if (nRevision >= kCurrentKeyRevision) {
        char chFollowPath = '\0';
        stream.ReadBytes(&chFollowPath, sizeof(chFollowPath));
        mFollowPath = chFollowPath != '\0';
    } else {
        if (mTransInterp == kInterpLinear) {
            mTransKeys.clear();
            for (const auto &legacy : legacyTransKeys) {
                AppendLegacyTransKey(mTransKeys, legacy);
            }
        }
        if (mRotInterp == kInterpLinear) {
            mRotKeys.clear();
            for (const auto &legacy : legacyRotKeys) {
                AppendLegacyRotKey(mRotKeys, legacy);
            }
        }
        if (mScaleInterp == kInterpLinear) {
            mScaleKeys.clear();
            for (const auto &legacy : legacyScaleKeys) {
                AppendLegacyTransKey(mScaleKeys, legacy);
            }
        }
        mFollowPath = mFramesOwner->mRotKeys.empty() && mFramesOwner->mTransKeys.size() >= 2;
    }

    if (nRevision <= kTransAnimRevision) { // Yes, the test cannot fail here.
        ClearKeys();
    }
    AddObjectRefs();
}

} // namespace Rnd
