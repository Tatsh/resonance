#include "rnd/animatable.h"

#include <algorithm>
#include <list>
#include <math.h>

#include "os/dbg.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

// The only revision this build writes, and the highest it accepts.
constexpr int kAnimatableRevision = 0;

constexpr char kAlreadyInFormat[] = "%s already in %s\n";
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

// NTSC-U/C: 0x00497ed8, PAL: 0x004d5e08
static Dbg &operator<<(Dbg &sink, const std::list<Animatable::Filter *> &filters) {
    sink.Print("(size:");
    sink.Format(kSizeFormat, filters.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<Animatable::Filter *>::const_iterator it = filters.begin(); it != filters.end();
         ++it) {
        sink.Print("\n");
        sink.Format(kCountFormat, nIndex);
        sink.Print("\t");
        sink.Print("(type:");
        sink.Format(kCountFormat, (*it)->Type());
        sink.Print(" params:");
        (*it)->Dump(sink);
        sink.Print(")");
        ++nIndex;
    }
    return sink;
}

// NTSC-U/C: 0x00498078, PAL: 0x004d5fa8
static Dbg &operator<<(Dbg &sink, const std::list<Animatable *> &anims) {
    sink.Print("(size:");
    sink.Format(kSizeFormat, anims.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<Animatable *>::const_iterator it = anims.begin(); it != anims.end(); ++it) {
        sink.Print("\n");
        sink.Format(kCountFormat, nIndex);
        sink.Print("\t");
        if (*it != nullptr) {
            sink.Format(kQuotedTextFormat, NameText(*it));
        } else {
            sink.Print("no object");
        }
        ++nIndex;
    }
    return sink;
}

// NTSC-U/C: 0x0049a8a8, PAL: 0x004d8810
// No call site survives in the shipped build, and the five literals below are the only
// record of the FilterType names. The routine is not static: an out-of-line copy with no caller is
// what external linkage produces, where internal linkage would have let the compiler discard it.
Dbg &operator<<(Dbg &sink, Animatable::FilterType nType) {
    switch (nType) {
    case Animatable::kFilterScaleOffset:
        sink.Print("ScaleOffset");
        break;
    case Animatable::kFilterMinMaxLoop:
        sink.Print("MinMaxLoop");
        break;
    case Animatable::kFilterZeroOrder:
        sink.Print("ZeroOrder");
        break;
    case Animatable::kFilterFirstOrder:
        sink.Print("FirstOrder");
        break;
    case Animatable::kFilterSecondOrder:
        sink.Print("SecondOrder");
        break;
    }
    return sink;
}

// NTSC-U/C: 0x004981e0, PAL: 0x004d6110
//
// Each entry is written as its type tag followed by whatever that filter's own Save() emits. The
// reader has to build the object before it can read the payload.
static Stream &operator<<(Stream &stream, const std::list<Animatable::Filter *> &filters) {
    int nCount = filters.size();
    stream.Write(&nCount, sizeof(nCount));

    for (std::list<Animatable::Filter *>::const_iterator it = filters.begin(); it != filters.end();
         ++it) {
        int nType = (*it)->Type();
        stream.Write(&nType, sizeof(nType));
        (*it)->Save(stream);
    }
    return stream;
}

// NTSC-U/C: 0x004982e8, PAL: 0x004d6218
//
// Each entry is written as the referenced object's name including its terminator. A reader has to
// resolve the names through Rnd::g_manager. An empty entry writes one zero byte.
static Stream &operator<<(Stream &stream, const std::list<Animatable *> &anims) {
    int nCount = anims.size();
    stream.Write(&nCount, sizeof(nCount));

    for (std::list<Animatable *>::const_iterator it = anims.begin(); it != anims.end(); ++it) {
        const Object *pObject = *it;
        if (pObject != nullptr) {
            stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
        } else {
            const char cEmpty = 0;
            stream.WriteBytes(&cEmpty, sizeof(cEmpty));
        }
    }
    return stream;
}

// NTSC-U/C: 0x0049a838, PAL: 0x004d87a0
// The list reader below inlines this body rather than calling it.
static Stream &operator>>(Stream &stream, Animatable::Filter *&pFilter) {
    int nType = 0;
    stream.Read(&nType, sizeof(nType));
    pFilter = Animatable::NewFilter(nType);
    pFilter->Load(stream);
    return stream;
}

// NTSC-U/C: 0x004985c0, PAL: 0x004d64f0
static Stream &operator>>(Stream &stream, std::list<Animatable::Filter *> &filters) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    filters.resize(nCount, nullptr);

    for (std::list<Animatable::Filter *>::iterator it = filters.begin(); it != filters.end();
         ++it) {
        stream >> *it;
    }
    return stream;
}

// NTSC-U/C: 0x00498860, PAL: 0x004d6790
static Stream &operator>>(Stream &stream, std::list<Animatable *> &anims) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    anims.resize(nCount, nullptr);

    for (std::list<Animatable *>::iterator it = anims.begin(); it != anims.end(); ++it) {
        HxStr name(nullptr);
        stream.ReadString(name);
        Object *pObject = g_manager.Find(name);
        *it = dynamic_cast<Animatable *>(pObject);
    }
    return stream;
}

// NTSC-U/C: 0x0049a560, PAL: 0x004d84c8
float Animatable::Filter::Inverse(float flValue) {
    return flValue;
}

// NTSC-U/C: 0x00498ef0, PAL: 0x004d6e50
float Animatable::ScaleOffset::Apply(float flValue) {
    return (flValue * mScale) + mOffset;
}

// NTSC-U/C: 0x00498f08, PAL: 0x004d6e68
float Animatable::ScaleOffset::Inverse(float flValue) {
    return (flValue - mOffset) / mScale;
}

// NTSC-U/C: 0x00498f20, PAL: 0x004d6e80
void Animatable::ScaleOffset::Dump(Dbg &sink) {
    sink.Print("(scale:");
    sink.Format(kFloatFormat, mScale);
    sink.Print(" offset:");
    sink.Format(kFloatFormat, mOffset);
    sink.Print(")");
}

// NTSC-U/C: 0x00498fc0, PAL: 0x004d6f20
void Animatable::ScaleOffset::Save(Stream &stream) {
    float flScale = mScale;
    stream.Write(&flScale, sizeof(flScale));
    float flOffset = mOffset;
    stream.Write(&flOffset, sizeof(flOffset));
}

// NTSC-U/C: 0x00499030, PAL: 0x004d6f90
void Animatable::ScaleOffset::Load(Stream &stream) {
    stream.Read(&mScale, sizeof(mScale));
    stream.Read(&mOffset, sizeof(mOffset));
}

// NTSC-U/C: 0x00499090, PAL: 0x004d6ff0
int Animatable::ScaleOffset::Type() {
    return kFilterScaleOffset;
}

// NTSC-U/C: 0x00499098, PAL: 0x004d6ff8
// Compiled as a block copy of all twelve bytes, the vtable pointer included.
void Animatable::ScaleOffset::Copy(const Filter *pSource) {
    *this = *static_cast<const ScaleOffset *>(pSource);
}

// NTSC-U/C: 0x0049a4b8, PAL: 0x004d8420
float Animatable::MinMaxLoop::Apply(float flValue) {
    if (mLoop != 0) {
        const float flSpan = mMax - mMin;
        float flWrapped = fmodf(flValue - mMin, flSpan);
        if (flWrapped < 0.0f) {
            flWrapped += flSpan;
        }
        return mMin + flWrapped;
    }
    return std::max(std::min(flValue, mMax), mMin);
}

// NTSC-U/C: 0x004990f0, PAL: 0x004d7050
void Animatable::MinMaxLoop::Dump(Dbg &sink) {
    sink.Print("(min:");
    sink.Format(kFloatFormat, mMin);
    sink.Print(" max:");
    sink.Format(kFloatFormat, mMax);
    sink.Print(" loop:");
    sink.Print((mLoop != 0 ? kTrueText : kFalseText));
    sink.Print(")");
}

// NTSC-U/C: 0x004991c8, PAL: 0x004d7128
void Animatable::MinMaxLoop::Save(Stream &stream) {
    float flMin = mMin;
    stream.Write(&flMin, sizeof(flMin));
    float flMax = mMax;
    stream.Write(&flMax, sizeof(flMax));

    const char cLoop = static_cast<char>(mLoop);
    stream.WriteBytes(&cLoop, sizeof(cLoop));
}

// NTSC-U/C: 0x00499258, PAL: 0x004d71b8
void Animatable::MinMaxLoop::Load(Stream &stream) {
    stream.Read(&mMin, sizeof(mMin));
    stream.Read(&mMax, sizeof(mMax));

    char cLoop = 0;
    stream.ReadBytes(&cLoop, sizeof(cLoop));
    mLoop = cLoop != 0 ? 1 : 0;
}

// NTSC-U/C: 0x004992e0, PAL: 0x004d7240
int Animatable::MinMaxLoop::Type() {
    return kFilterMinMaxLoop;
}

// NTSC-U/C: 0x004992e8, PAL: 0x004d7248
// Compiled as a block copy of all sixteen bytes, the vtable pointer included.
void Animatable::MinMaxLoop::Copy(const Filter *pSource) {
    *this = *static_cast<const MinMaxLoop *>(pSource);
}

// NTSC-U/C: 0x0049a568, PAL: 0x004d84d0
float Animatable::ZeroOrder::Apply(float flValue) {
    const float flDelta = flValue - mLevel;
    if (mMaxDelta < flDelta) {
        mLevel += mMaxDelta;
    } else if (flDelta < -mMaxDelta) {
        mLevel -= mMaxDelta;
    } else {
        mLevel = flValue;
    }
    return mLevel;
}

// NTSC-U/C: 0x0049a5b8, PAL: 0x004d8520
float Animatable::ZeroOrder::Inverse(float flValue) {
    if (flValue < mLevel) {
        return mLevel + mMaxDelta;
    }
    if (mLevel < flValue) {
        return mLevel - mMaxDelta;
    }
    return mLevel;
}

// NTSC-U/C: 0x00499348, PAL: 0x004d72a8
void Animatable::ZeroOrder::Dump(Dbg &sink) {
    sink.Print("(level:");
    sink.Format(kFloatFormat, mLevel);
    sink.Print(" maxDelta:");
    sink.Format(kFloatFormat, mMaxDelta);
    sink.Print(")");
}

// NTSC-U/C: 0x004993e8, PAL: 0x004d7348
void Animatable::ZeroOrder::Save(Stream &stream) {
    float flLevel = mLevel;
    stream.Write(&flLevel, sizeof(flLevel));
    float flMaxDelta = mMaxDelta;
    stream.Write(&flMaxDelta, sizeof(flMaxDelta));
}

// NTSC-U/C: 0x00499458, PAL: 0x004d73b8
void Animatable::ZeroOrder::Load(Stream &stream) {
    stream.Read(&mLevel, sizeof(mLevel));
    stream.Read(&mMaxDelta, sizeof(mMaxDelta));
}

// NTSC-U/C: 0x004994b8, PAL: 0x004d7418
int Animatable::ZeroOrder::Type() {
    return kFilterZeroOrder;
}

// NTSC-U/C: 0x004994c0, PAL: 0x004d7420
// Compiled as a block copy of all twelve bytes, the vtable pointer included.
void Animatable::ZeroOrder::Copy(const Filter *pSource) {
    *this = *static_cast<const ZeroOrder *>(pSource);
}

// NTSC-U/C: 0x004994e0, PAL: 0x004d7440
float Animatable::FirstOrder::Apply(float flValue) {
    mLevel += (flValue - mLevel) * mRatio;
    return mLevel;
}

// NTSC-U/C: 0x00499500, PAL: 0x004d7460
float Animatable::FirstOrder::Inverse(float flValue) {
    return (mLevel - (flValue * mRatio)) / (1.0f - mRatio);
}

// NTSC-U/C: 0x00499528, PAL: 0x004d7488
void Animatable::FirstOrder::Dump(Dbg &sink) {
    sink.Print("(level:");
    sink.Format(kFloatFormat, mLevel);
    sink.Print(" ratio:");
    sink.Format(kFloatFormat, mRatio);
    sink.Print(")");
}

// NTSC-U/C: 0x004995c8, PAL: 0x004d7528
void Animatable::FirstOrder::Save(Stream &stream) {
    float flLevel = mLevel;
    stream.Write(&flLevel, sizeof(flLevel));
    float flRatio = mRatio;
    stream.Write(&flRatio, sizeof(flRatio));
}

// NTSC-U/C: 0x00499638, PAL: 0x004d7598
void Animatable::FirstOrder::Load(Stream &stream) {
    stream.Read(&mLevel, sizeof(mLevel));
    stream.Read(&mRatio, sizeof(mRatio));
}

// NTSC-U/C: 0x00499698, PAL: 0x004d75f8
int Animatable::FirstOrder::Type() {
    return kFilterFirstOrder;
}

// NTSC-U/C: 0x004996a0, PAL: 0x004d7600
// Compiled as a block copy of all twelve bytes, the vtable pointer included.
void Animatable::FirstOrder::Copy(const Filter *pSource) {
    *this = *static_cast<const FirstOrder *>(pSource);
}

// NTSC-U/C: 0x0049a5f8, PAL: 0x004d8560
float Animatable::SecondOrder::Apply(float flValue) {
    mVel += (mSpring * (flValue - mLevel)) - (mDamper * mVel);
    mLevel += mVel;
    return mLevel;
}

// NTSC-U/C: 0x0049a630, PAL: 0x004d8598
float Animatable::SecondOrder::Inverse([[maybe_unused]] float flValue) {
    return mLevel - mVel;
}

// NTSC-U/C: 0x00499700, PAL: 0x004d7660
void Animatable::SecondOrder::Dump(Dbg &sink) {
    sink.Print("(level:");
    sink.Format(kFloatFormat, mLevel);
    sink.Print(" spring:");
    sink.Format(kFloatFormat, mSpring);
    sink.Print(" damper:");
    sink.Print(")"); // Yes, the closing bracket lands here rather than at the end.
    sink.Format(kFloatFormat, mDamper);
    sink.Print(" vel");
    sink.Format(kFloatFormat, mVel);
}

// NTSC-U/C: 0x00499800, PAL: 0x004d7760
void Animatable::SecondOrder::Save(Stream &stream) {
    float flLevel = mLevel;
    stream.Write(&flLevel, sizeof(flLevel));
    float flSpring = mSpring;
    stream.Write(&flSpring, sizeof(flSpring));
    float flDamper = mDamper;
    stream.Write(&flDamper, sizeof(flDamper));
    // Yes, mVel is dumped but never written. A saved stage reloads with whatever velocity the
    // reading object already had.
}

// NTSC-U/C: 0x00499890, PAL: 0x004d77f0
void Animatable::SecondOrder::Load(Stream &stream) {
    stream.Read(&mLevel, sizeof(mLevel));
    stream.Read(&mSpring, sizeof(mSpring));
    stream.Read(&mDamper, sizeof(mDamper));
}

// NTSC-U/C: 0x00499908, PAL: 0x004d7868
int Animatable::SecondOrder::Type() {
    return kFilterSecondOrder;
}

// NTSC-U/C: 0x00499910, PAL: 0x004d7870
// Compiled as a block copy of all twenty bytes, the vtable pointer included.
void Animatable::SecondOrder::Copy(const Filter *pSource) {
    *this = *static_cast<const SecondOrder *>(pSource);
}

// NTSC-U/C: 0x0049a108, PAL: 0x004d8068
Animatable::Animatable() : mFrame(0.0f), mFilteredFrame(0.0f) {
}

// NTSC-U/C: 0x00499f48, PAL: 0x004d7ea8
Animatable::~Animatable() {
    ReleaseAnimsAndFilters();
}

// NTSC-U/C: 0x00494cb0, PAL: 0x004d2b78
void Animatable::ReleaseAnimsAndFilters() {
    // Unlike ReleaseAnimsRefs(), the walk leaves mAnims populated.
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
    }
    for (std::list<Filter *>::iterator it = mFilters.begin(); it != mFilters.end(); ++it) {
        // Filter declares no destructor, so every subclass is released through a base pointer
        // without one being run. 0x00494d2c calls the global operator delete on the pointer
        // directly, with no vptr load and no dispatch, which confirms the original has the same
        // defect rather than this being a reconstruction error. The diagnostic is suppressed at the
        // two sites that reproduce it instead of over the file.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
        delete *it;
#pragma GCC diagnostic pop
    }
    mFilters.clear();
}

// NTSC-U/C: 0x00494a88, PAL: 0x004d2950
Animatable *Animatable::Parent() {
    for (std::list<Object *>::iterator it = mRefs.begin(); it != mRefs.end(); ++it) {
        Animatable *pCandidate = dynamic_cast<Animatable *>(*it);
        if (pCandidate == nullptr) {
            continue;
        }
        if (std::find(pCandidate->mAnims.begin(), pCandidate->mAnims.end(), this) !=
            pCandidate->mAnims.end()) {
            return pCandidate;
        }
    }
    return nullptr;
}

// NTSC-U/C: 0x004953c0, PAL: 0x004d3288
void Animatable::AddAnim(Animatable *pAnim) {
    if (std::find(mAnims.begin(), mAnims.end(), pAnim) != mAnims.end()) {
        g_failSink.Report(kAlreadyInFormat, NameText(pAnim), NameText(this));
        return;
    }

    if (pAnim != nullptr) {
        pAnim->AddRef(this);
    }
    mAnims.push_back(pAnim);
}

// NTSC-U/C: 0x00495540, PAL: 0x004d3408
void Animatable::RemoveAnim(Animatable *pAnim) {
    if (std::find(mAnims.begin(), mAnims.end(), pAnim) == mAnims.end()) {
        return;
    }

    if (pAnim != nullptr) {
        pAnim->RemoveRef(this);
    }
    mAnims.remove(pAnim);
}

// NTSC-U/C: 0x0049a428, PAL: 0x004d8390
void Animatable::SetFrame(float flFrame) {
    mFrame = flFrame;
    mFilteredFrame = ApplyFilters(flFrame);
    SetFrameSelf(mFilteredFrame);

    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        (*it)->SetFrame(mFilteredFrame);
    }
}

// NTSC-U/C: 0x0049a100, PAL: 0x004d8060
void Animatable::SetFrameSelf([[maybe_unused]] float flFrame) {
}

// NTSC-U/C: 0x00494b50, PAL: 0x004d2a18
float Animatable::EndFrame() {
    float flEnd = 0.0f;
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        flEnd = std::max(flEnd, (*it)->InverseFilters((*it)->EndFrame()));
    }
    return flEnd;
}

// NTSC-U/C: 0x0049a3b8, PAL: 0x004d8320
void Animatable::StartAnim() {
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        (*it)->StartAnim();
    }
}

// NTSC-U/C: 0x0049a7c0, PAL: 0x004d8728
float Animatable::ApplyFilters(float flValue) {
    for (std::list<Filter *>::iterator it = mFilters.begin(); it != mFilters.end(); ++it) {
        flValue = (*it)->Apply(flValue);
    }
    return flValue;
}

// NTSC-U/C: 0x00495050, PAL: 0x004d2f18
float Animatable::InverseFilters(float flValue) {
    for (std::list<Filter *>::reverse_iterator it = mFilters.rbegin(); it != mFilters.rend();
         ++it) {
        flValue = (*it)->Inverse(flValue);
    }
    return flValue;
}

// NTSC-U/C: 0x00499940, PAL: 0x004d78a0
void Animatable::AddFilter(Filter *pFilter) {
    mFilters.push_back(pFilter);
}

// NTSC-U/C: 0x004999e8, PAL: 0x004d7948
void Animatable::AddScaleOffset(float flScale, float flOffset) {
    mFilters.push_back(new ScaleOffset(flScale, flOffset));
}

// NTSC-U/C: 0x00499ae0, PAL: 0x004d7a40
void Animatable::AddMinMaxLoop(float flMin, float flMax, int nLoop) {
    mFilters.push_back(new MinMaxLoop(flMin, flMax, nLoop));
}

// NTSC-U/C: 0x00499be8, PAL: 0x004d7b48
void Animatable::AddZeroOrder(float flLevel, float flMaxDelta) {
    mFilters.push_back(new ZeroOrder(flLevel, flMaxDelta));
}

// NTSC-U/C: 0x00499ce0, PAL: 0x004d7c40
void Animatable::AddFirstOrder(float flLevel, float flRatio) {
    mFilters.push_back(new FirstOrder(flLevel, flRatio));
}

// NTSC-U/C: 0x00499dd8, PAL: 0x004d7d38
void Animatable::AddSecondOrder(float flLevel, float flSpring, float flDamper) {
    mFilters.push_back(new SecondOrder(flLevel, flSpring, flDamper));
}

// NTSC-U/C: 0x00494c00, PAL: 0x004d2ac8
void Animatable::RemoveFilter(int nIndex) {
    std::list<Filter *>::iterator it = mFilters.begin();
    for (int i = 0; i < nIndex && it != mFilters.end(); ++i) {
        ++it;
    }
    if (it == mFilters.end()) {
        return;
    }

    // See ReleaseAnimsAndFilters() for why this deletion is faithful rather than defective here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
    delete *it;
#pragma GCC diagnostic pop
    mFilters.erase(it);
}

// NTSC-U/C: 0x0049a0b0, PAL: 0x004d8010
Animatable::Filter *Animatable::FilterAt(int nIndex) {
    std::list<Filter *>::iterator it = mFilters.begin();
    // Yes, the binary counts the index down to zero. A negative index walks forward.
    for (int i = nIndex; i != 0; --i) {
        ++it;
    }
    return *it;
}

// NTSC-U/C: 0x004950e8, PAL: 0x004d2fb0
Animatable::Filter *Animatable::NewFilter(int nType) {
    switch (nType) {
    case kFilterScaleOffset:
        return new ScaleOffset;
    case kFilterMinMaxLoop:
        return new MinMaxLoop;
    case kFilterZeroOrder:
        return new ZeroOrder;
    case kFilterFirstOrder:
        return new FirstOrder;
    case kFilterSecondOrder:
        return new SecondOrder;
    }

    // An unrecognised tag runs the abort handler with no message of its own.
    if (g_failSink.mAbortProc != nullptr) {
        g_failSink.mAbortProc();
    } else {
        throw; // With no handler the binary rethrows the exception in flight.
    }
    return nullptr;
}

// NTSC-U/C: 0x0049a960, PAL: 0x004d88c8
void Animatable::ReleaseAnimsRefs() {
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
    }
    mAnims.clear();
}

// NTSC-U/C: 0x0049a750, PAL: 0x004d86b8
void Animatable::AcquireAnimsRefs() {
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        if (*it != nullptr) {
            (*it)->AddRef(this);
        }
    }
}

// NTSC-U/C: 0x0049a640, PAL: 0x004d85a8
void Animatable::DumpText(Dbg &sink) {
    if (sink.mDumpLevel <= 0) {
        return;
    }
    sink.Print("[Animatable]\n");
    sink.Print("filters:");
    sink << mFilters;
    sink.Print("\n");
    sink.Print("anims:");
    sink << mAnims;
    sink.Print("\n");
}

// NTSC-U/C: 0x0049a6e8, PAL: 0x004d8650
void Animatable::Save(Stream &stream) {
    int nRevision = kAnimatableRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    stream << mFilters;
    stream << mAnims;
}

// NTSC-U/C: 0x00494d68, PAL: 0x004d2c30
void Animatable::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kAnimatableRevision) {
        g_failSink.Report("Can't load new Animatable\n");
        if (g_failSink.mAbortProc != nullptr) {
            g_failSink.mAbortProc();
        } else {
            throw;
        }
    }

    ReleaseAnimsAndFilters();

    stream >> mFilters;
    stream >> mAnims;
    AcquireAnimsRefs();
}

// NTSC-U/C: 0x00494e70, PAL: 0x004d2d38
void Animatable::Copy(const Object *pSource, unsigned nFlags) {
    const Animatable *pSourceAnim = dynamic_cast<const Animatable *>(pSource);

    ReleaseAnimsAndFilters();

    // A filter is cloned by tag rather than by a virtual clone, because Filter::Copy() only moves
    // the parameters of an object that already has the right type.
    for (std::list<Filter *>::const_iterator it = pSourceAnim->mFilters.begin();
         it != pSourceAnim->mFilters.end();
         ++it) {
        Filter *pCopy = NewFilter((*it)->Type());
        pCopy->Copy(*it);
        mFilters.push_back(pCopy);
    }

    if ((nFlags & kCopyChildLists) != 0) {
        mAnims = pSourceAnim->mAnims;
    }
    AcquireAnimsRefs();
}

// NTSC-U/C: 0x004951e0, PAL: 0x004d30a8
void Animatable::Replace(Object *pFrom, Object *pTo) {
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end();) {
        if (*it == pTo) {
            g_failSink.Report(kAlreadyInFormat, NameText(pTo), NameText(this));
        }

        if (*it == pFrom) {
            if (pFrom != nullptr) {
                pFrom->RemoveRef(this);
            }
            if (*it != nullptr) {
                *it = dynamic_cast<Animatable *>(pTo);
            }
            if (*it != nullptr) {
                (*it)->AddRef(this);
            }
        }

        if (*it == nullptr) {
            it = mAnims.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace Rnd
