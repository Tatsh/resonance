#include "rnd/drawable.h"

#include <algorithm>
#include <list>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

// The only revision this build writes, and the highest it accepts.
constexpr int kDrawableRevision = 0;

constexpr char kAlreadyInFormat[] = "%s already in %s\n";
constexpr char kCountFormat[] = "%d";
constexpr char kQuotedTextFormat[] = "\"%s\"";
constexpr char kTrueText[] = "true";
constexpr char kFalseText[] = "false";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
static const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

// 0x00505c78
static FailSink &operator<<(FailSink &sink, const std::list<Drawable *> &draws) {
    sink.Print("(size:");
    sink.Format(kCountFormat, draws.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<Drawable *>::const_iterator it = draws.begin(); it != draws.end(); ++it) {
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

// 0x00505fe8
//
// Each entry is written as the referenced object's name including its terminator, so a reader has
// to resolve the names through Rnd::g_manager. An empty entry writes one zero byte.
static Stream &operator<<(Stream &stream, const std::list<Drawable *> &draws) {
    int nCount = draws.size();
    stream.Write(&nCount, sizeof(nCount));

    for (std::list<Drawable *>::const_iterator it = draws.begin(); it != draws.end(); ++it) {
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

// 0x005062c0
static Stream &operator>>(Stream &stream, std::list<Drawable *> &draws) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    draws.resize(nCount, nullptr);

    for (std::list<Drawable *>::iterator it = draws.begin(); it != draws.end(); ++it) {
        HxStr name(nullptr);
        stream.ReadString(name);
        Object *pObject = g_manager.Find(name);
        *it = dynamic_cast<Drawable *>(pObject);
    }
    return stream;
}

// 0x005066f8
Drawable::Drawable() : mShowing(1), mHighlight(0) {
}

// 0x00506590
Drawable::~Drawable() {
    ReleaseDrawsRefs();
}

// 0x00506920
void Drawable::Draw() {
    if (mShowing == 0) {
        return;
    }
    if (DrawSelf() == 0) {
        return;
    }
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end(); ++it) {
        (*it)->Draw();
    }
}

// 0x005066d8
void Drawable::SetShowing(int nShowing) {
    mShowing = nShowing;
}

// 0x00506c88
void Drawable::SetHighlight(int nHighlight) {
    mHighlight = nHighlight;
}

// 0x005066f0
int Drawable::DrawSelf() {
    return 1;
}

// 0x00502d78
Drawable *Drawable::Parent() {
    for (std::list<Object *>::iterator it = mRefs.begin(); it != mRefs.end(); ++it) {
        Drawable *pCandidate = dynamic_cast<Drawable *>(*it);
        if (pCandidate == nullptr) {
            continue;
        }
        if (std::find(pCandidate->mDraws.begin(), pCandidate->mDraws.end(), this) !=
            pCandidate->mDraws.end()) {
            return pCandidate;
        }
    }
    return nullptr;
}

// 0x00503188
void Drawable::AddDraw(Drawable *pDraw, Drawable *pBefore) {
    if (std::find(mDraws.begin(), mDraws.end(), pDraw) != mDraws.end()) {
        g_failSink.Report(kAlreadyInFormat, NameText(pDraw), NameText(this));
        return;
    }

    std::list<Drawable *>::iterator at = std::find(mDraws.begin(), mDraws.end(), pBefore);
    if (pDraw != nullptr) {
        pDraw->AddRef(this);
    }
    mDraws.insert(at, pDraw);
}

// 0x005064e0
void Drawable::AddDraw(Drawable *pDraw) {
    AddDraw(pDraw, mDraws.empty() ? nullptr : mDraws.front());
}

// 0x00503420
void Drawable::ClearDraws() {
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end();) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
        it = mDraws.erase(it);
    }
}

// 0x00506ba8
void Drawable::ReleaseDrawsRefs() {
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end(); ++it) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
    }
}

// 0x00506c18
void Drawable::AcquireDrawsRefs() {
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end(); ++it) {
        if (*it != nullptr) {
            (*it)->AddRef(this);
        }
    }
}

// 0x005069a8
void Drawable::DumpText(FailSink &sink) {
    if (sink.mDumpLevel <= 0) {
        return;
    }
    sink.Print("[Drawable]\n");
    sink.Print("show:");
    sink.Print((mShowing != 0 ? kTrueText : kFalseText));
    sink.Print(" highlight:");
    sink.Print((mHighlight != 0 ? kTrueText : kFalseText));
    sink.Print(" draws:");
    sink << mDraws;
    sink.Print("\n");
}

// 0x00506b28
void Drawable::Save(Stream &stream) {
    int nRevision = kDrawableRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    const char cShowing = static_cast<char>(mShowing);
    stream.WriteBytes(&cShowing, sizeof(cShowing));

    stream << mDraws;
}

// 0x00503020
void Drawable::Load(Stream &stream) {
    int nRevision = 0;
    stream.Read(&nRevision, sizeof(nRevision));
    if (nRevision > kDrawableRevision) {
        g_failSink.Report("Can't load new Drawable\n");
        if (g_failSink.mAbortProc != nullptr) {
            g_failSink.mAbortProc();
        }
    }

    ReleaseDrawsRefs();

    char cShowing = 0;
    stream.ReadBytes(&cShowing, sizeof(cShowing));
    mShowing = cShowing != 0 ? 1 : 0;

    stream >> mDraws;
    AcquireDrawsRefs();
}

// 0x00506a88
void Drawable::Copy(const Object *pSource, unsigned nFlags) {
    const Drawable *pSourceDrawable = dynamic_cast<const Drawable *>(pSource);

    ReleaseDrawsRefs();
    mShowing = pSourceDrawable->mShowing; // Yes, the binary reads this without a null check.
    if ((nFlags & kCopyChildLists) != 0) {
        mDraws = pSourceDrawable->mDraws;
    }
    AcquireDrawsRefs();
}

// 0x00502e40
void Drawable::Replace(Object *pFrom, Object *pTo) {
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end();) {
        if (*it == pTo) {
            g_failSink.Report(kAlreadyInFormat, NameText(pTo), NameText(this));
        }

        if (*it == pFrom) {
            if (pFrom != nullptr) {
                pFrom->RemoveRef(this);
            }
            if (*it != nullptr) {
                *it = dynamic_cast<Drawable *>(pTo);
            }
            if (*it != nullptr) {
                (*it)->AddRef(this);
            }
        }

        if (*it == nullptr) {
            it = mDraws.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace Rnd
