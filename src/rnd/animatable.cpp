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
        (*it)->Print(sink);
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
    stream.WriteLE(&nCount, sizeof(nCount));

    for (std::list<Animatable::Filter *>::const_iterator it = filters.begin(); it != filters.end();
         ++it) {
        int nType = (*it)->Type();
        stream.WriteLE(&nType, sizeof(nType));
        (*it)->Save(stream);
    }
    return stream;
}

// NTSC-U/C: 0x004982e8, PAL: 0x004d6218
//
// Each entry is written as the referenced object's name including its terminator. A reader has to
// resolve the names through Rnd::TheManager. An empty entry writes one zero byte.
static Stream &operator<<(Stream &stream, const std::list<Animatable *> &anims) {
    int nCount = anims.size();
    stream.WriteLE(&nCount, sizeof(nCount));

    for (std::list<Animatable *>::const_iterator it = anims.begin(); it != anims.end(); ++it) {
        const Object *pObject = *it;
        if (pObject != nullptr) {
            stream.Write(NameText(pObject), pObject->mName.mLen + 1);
        } else {
            const char cEmpty = 0;
            stream.Write(&cEmpty, sizeof(cEmpty));
        }
    }
    return stream;
}

// NTSC-U/C: 0x0049a838, PAL: 0x004d87a0
// The list reader below inlines this body rather than calling it.
static Stream &operator>>(Stream &stream, Animatable::Filter *&pFilter) {
    int nType = 0;
    stream.ReadLE(&nType, sizeof(nType));
    pFilter = Animatable::NewFilter(nType);
    pFilter->Load(stream);
    return stream;
}

// NTSC-U/C: 0x004985c0, PAL: 0x004d64f0
static Stream &operator>>(Stream &stream, std::list<Animatable::Filter *> &filters) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
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
    stream.ReadLE(&nCount, sizeof(nCount));
    anims.resize(nCount, nullptr);

    for (std::list<Animatable *>::iterator it = anims.begin(); it != anims.end(); ++it) {
        HxStr name(nullptr);
        stream.ReadString(name);
        Object *pObject = TheManager.Find(name);
        *it = dynamic_cast<Animatable *>(pObject);
    }
    return stream;
}

float Animatable::Filter::Unapply(float flValue) {
    return flValue;
}

float Animatable::ScaleOffset::Apply(float flValue) {
    return (flValue * mScale) + mOffset;
}

float Animatable::ScaleOffset::Unapply(float flValue) {
    return (flValue - mOffset) / mScale;
}

void Animatable::ScaleOffset::Print(Dbg &sink) const {
    sink.Print("(scale:");
    sink.Format(kFloatFormat, mScale);
    sink.Print(" offset:");
    sink.Format(kFloatFormat, mOffset);
    sink.Print(")");
}

void Animatable::ScaleOffset::Save(Stream &stream) {
    float flScale = mScale;
    stream.WriteLE(&flScale, sizeof(flScale));
    float flOffset = mOffset;
    stream.WriteLE(&flOffset, sizeof(flOffset));
}

void Animatable::ScaleOffset::Load(Stream &stream) {
    stream.ReadLE(&mScale, sizeof(mScale));
    stream.ReadLE(&mOffset, sizeof(mOffset));
}

int Animatable::ScaleOffset::Type() {
    return kFilterScaleOffset;
}

void Animatable::ScaleOffset::Copy(const Filter *pSource) {
    // Compiled as a block copy of all twelve bytes, the vtable pointer included.
    *this = *static_cast<const ScaleOffset *>(pSource);
}

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

void Animatable::MinMaxLoop::Print(Dbg &sink) const {
    sink.Print("(min:");
    sink.Format(kFloatFormat, mMin);
    sink.Print(" max:");
    sink.Format(kFloatFormat, mMax);
    sink.Print(" loop:");
    sink.Print((mLoop != 0 ? kTrueText : kFalseText));
    sink.Print(")");
}

void Animatable::MinMaxLoop::Save(Stream &stream) {
    float flMin = mMin;
    stream.WriteLE(&flMin, sizeof(flMin));
    float flMax = mMax;
    stream.WriteLE(&flMax, sizeof(flMax));

    const char cLoop = static_cast<char>(mLoop);
    stream.Write(&cLoop, sizeof(cLoop));
}

void Animatable::MinMaxLoop::Load(Stream &stream) {
    stream.ReadLE(&mMin, sizeof(mMin));
    stream.ReadLE(&mMax, sizeof(mMax));

    char cLoop = 0;
    stream.Read(&cLoop, sizeof(cLoop));
    mLoop = cLoop != 0 ? 1 : 0;
}

int Animatable::MinMaxLoop::Type() {
    return kFilterMinMaxLoop;
}

void Animatable::MinMaxLoop::Copy(const Filter *pSource) {
    // Compiled as a block copy of all sixteen bytes, the vtable pointer included.
    *this = *static_cast<const MinMaxLoop *>(pSource);
}

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

float Animatable::ZeroOrder::Unapply(float flValue) {
    if (flValue < mLevel) {
        return mLevel + mMaxDelta;
    }
    if (mLevel < flValue) {
        return mLevel - mMaxDelta;
    }
    return mLevel;
}

void Animatable::ZeroOrder::Print(Dbg &sink) const {
    sink.Print("(level:");
    sink.Format(kFloatFormat, mLevel);
    sink.Print(" maxDelta:");
    sink.Format(kFloatFormat, mMaxDelta);
    sink.Print(")");
}

void Animatable::ZeroOrder::Save(Stream &stream) {
    float flLevel = mLevel;
    stream.WriteLE(&flLevel, sizeof(flLevel));
    float flMaxDelta = mMaxDelta;
    stream.WriteLE(&flMaxDelta, sizeof(flMaxDelta));
}

void Animatable::ZeroOrder::Load(Stream &stream) {
    stream.ReadLE(&mLevel, sizeof(mLevel));
    stream.ReadLE(&mMaxDelta, sizeof(mMaxDelta));
}

int Animatable::ZeroOrder::Type() {
    return kFilterZeroOrder;
}

void Animatable::ZeroOrder::Copy(const Filter *pSource) {
    // Compiled as a block copy of all twelve bytes, the vtable pointer included.
    *this = *static_cast<const ZeroOrder *>(pSource);
}

float Animatable::FirstOrder::Apply(float flValue) {
    mLevel += (flValue - mLevel) * mRatio;
    return mLevel;
}

float Animatable::FirstOrder::Unapply(float flValue) {
    return (mLevel - (flValue * mRatio)) / (1.0f - mRatio);
}

void Animatable::FirstOrder::Print(Dbg &sink) const {
    sink.Print("(level:");
    sink.Format(kFloatFormat, mLevel);
    sink.Print(" ratio:");
    sink.Format(kFloatFormat, mRatio);
    sink.Print(")");
}

void Animatable::FirstOrder::Save(Stream &stream) {
    float flLevel = mLevel;
    stream.WriteLE(&flLevel, sizeof(flLevel));
    float flRatio = mRatio;
    stream.WriteLE(&flRatio, sizeof(flRatio));
}

void Animatable::FirstOrder::Load(Stream &stream) {
    stream.ReadLE(&mLevel, sizeof(mLevel));
    stream.ReadLE(&mRatio, sizeof(mRatio));
}

int Animatable::FirstOrder::Type() {
    return kFilterFirstOrder;
}

void Animatable::FirstOrder::Copy(const Filter *pSource) {
    // Compiled as a block copy of all twelve bytes, the vtable pointer included.
    *this = *static_cast<const FirstOrder *>(pSource);
}

float Animatable::SecondOrder::Apply(float flValue) {
    mVel += (mSpring * (flValue - mLevel)) - (mDamper * mVel);
    mLevel += mVel;
    return mLevel;
}

float Animatable::SecondOrder::Unapply([[maybe_unused]] float flValue) {
    return mLevel - mVel;
}

void Animatable::SecondOrder::Print(Dbg &sink) const {
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

void Animatable::SecondOrder::Save(Stream &stream) {
    float flLevel = mLevel;
    stream.WriteLE(&flLevel, sizeof(flLevel));
    float flSpring = mSpring;
    stream.WriteLE(&flSpring, sizeof(flSpring));
    float flDamper = mDamper;
    stream.WriteLE(&flDamper, sizeof(flDamper));
    // Yes, mVel is dumped but never written. A saved stage reloads with whatever velocity the
    // reading object already had.
}

void Animatable::SecondOrder::Load(Stream &stream) {
    stream.ReadLE(&mLevel, sizeof(mLevel));
    stream.ReadLE(&mSpring, sizeof(mSpring));
    stream.ReadLE(&mDamper, sizeof(mDamper));
}

int Animatable::SecondOrder::Type() {
    return kFilterSecondOrder;
}

void Animatable::SecondOrder::Copy(const Filter *pSource) {
    // Compiled as a block copy of all twenty bytes, the vtable pointer included.
    *this = *static_cast<const SecondOrder *>(pSource);
}

Animatable::Animatable() : mFrame(0.0f), mFilteredFrame(0.0f) {
}

Animatable::~Animatable() {
    ReleaseObjects();
}

void Animatable::ReleaseObjects() {
    // Unlike RemoveAllAnims(), the walk leaves mAnims populated.
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

void Animatable::AddAnim(Animatable *pAnim) {
    if (std::find(mAnims.begin(), mAnims.end(), pAnim) != mAnims.end()) {
        Rnd::TheDbg.Notify(kAlreadyInFormat, NameText(pAnim), NameText(this));
        return;
    }

    if (pAnim != nullptr) {
        pAnim->AddRef(this);
    }
    mAnims.push_back(pAnim);
}

void Animatable::RemoveAnim(Animatable *pAnim) {
    if (std::find(mAnims.begin(), mAnims.end(), pAnim) == mAnims.end()) {
        return;
    }

    if (pAnim != nullptr) {
        pAnim->RemoveRef(this);
    }
    mAnims.remove(pAnim);
}

void Animatable::SetFrame(float flFrame) {
    mFrame = flFrame;
    mFilteredFrame = FilterFrame(flFrame);
    SetFrameSelf(mFilteredFrame);

    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        (*it)->SetFrame(mFilteredFrame);
    }
}

void Animatable::SetFrameSelf([[maybe_unused]] float flFrame) {
}

float Animatable::FilteredFrameEnd() {
    float flEnd = 0.0f;
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        flEnd = std::max(flEnd, (*it)->UnfilterFrame((*it)->FilteredFrameEnd()));
    }
    return flEnd;
}

void Animatable::StartAnim() {
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        (*it)->StartAnim();
    }
}

float Animatable::FilterFrame(float flValue) {
    for (std::list<Filter *>::iterator it = mFilters.begin(); it != mFilters.end(); ++it) {
        flValue = (*it)->Apply(flValue);
    }
    return flValue;
}

float Animatable::UnfilterFrame(float flValue) {
    for (std::list<Filter *>::reverse_iterator it = mFilters.rbegin(); it != mFilters.rend();
         ++it) {
        flValue = (*it)->Unapply(flValue);
    }
    return flValue;
}

void Animatable::AddFilter(Filter *pFilter) {
    mFilters.push_back(pFilter);
}

void Animatable::AddScaleOffset(float flScale, float flOffset) {
    mFilters.push_back(new ScaleOffset(flScale, flOffset));
}

void Animatable::AddMinMaxLoop(float flMin, float flMax, int nLoop) {
    mFilters.push_back(new MinMaxLoop(flMin, flMax, nLoop));
}

void Animatable::AddZeroOrder(float flLevel, float flMaxDelta) {
    mFilters.push_back(new ZeroOrder(flLevel, flMaxDelta));
}

void Animatable::AddFirstOrder(float flLevel, float flRatio) {
    mFilters.push_back(new FirstOrder(flLevel, flRatio));
}

void Animatable::AddSecondOrder(float flLevel, float flSpring, float flDamper) {
    mFilters.push_back(new SecondOrder(flLevel, flSpring, flDamper));
}

void Animatable::RemoveFilter(int nIndex) {
    std::list<Filter *>::iterator it = mFilters.begin();
    for (int i = 0; i < nIndex && it != mFilters.end(); ++i) {
        ++it;
    }
    if (it == mFilters.end()) {
        return;
    }

    // See ReleaseObjects() for why this deletion is faithful rather than defective here.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
    delete *it;
#pragma GCC diagnostic pop
    mFilters.erase(it);
}

Animatable::Filter *Animatable::FilterAt(int nIndex) {
    std::list<Filter *>::iterator it = mFilters.begin();
    // Yes, the binary counts the index down to zero. A negative index walks forward.
    for (int i = nIndex; i != 0; --i) {
        ++it;
    }
    return *it;
}

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
    if (Rnd::TheDbg.mAbortProc != nullptr) {
        Rnd::TheDbg.mAbortProc();
    } else {
        throw; // With no handler the binary rethrows the exception in flight.
    }
    return nullptr;
}

void Animatable::RemoveAllAnims() {
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
    }
    mAnims.clear();
}

void Animatable::AcquireAnimsRefs() {
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end(); ++it) {
        if (*it != nullptr) {
            (*it)->AddRef(this);
        }
    }
}

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

void Animatable::Save(Stream &stream) {
    int nRevision = kAnimatableRevision;
    stream.WriteLE(&nRevision, sizeof(nRevision));

    stream << mFilters;
    stream << mAnims;
}

void Animatable::Load(Stream &stream) {
    int nRevision = 0;
    stream.ReadLE(&nRevision, sizeof(nRevision));
    if (nRevision > kAnimatableRevision) {
        Rnd::TheDbg.Notify("Can't load new Animatable\n");
        if (Rnd::TheDbg.mAbortProc != nullptr) {
            Rnd::TheDbg.mAbortProc();
        } else {
            throw;
        }
    }

    ReleaseObjects();

    stream >> mFilters;
    stream >> mAnims;
    AcquireAnimsRefs();
}

void Animatable::Copy(const Object *pSource, unsigned nFlags) {
    const Animatable *pSourceAnim = dynamic_cast<const Animatable *>(pSource);

    ReleaseObjects();

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

void Animatable::Replace(Object *pFrom, Object *pTo) {
    for (std::list<Animatable *>::iterator it = mAnims.begin(); it != mAnims.end();) {
        if (*it == pTo) {
            Rnd::TheDbg.Notify(kAlreadyInFormat, NameText(pTo), NameText(this));
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
