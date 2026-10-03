#include "rnd/drawable.h"

#include <algorithm>
#include <list>

#include "os/dbg.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

// The only revision this build writes, and the highest it accepts.
constexpr int kDrawableRevision = 0;

constexpr char kAlreadyInFormat[] = "%s already in %s\n";
constexpr char kCountFormat[] = "%d";
constexpr char kSizeFormat[] = "%u";
constexpr char kQuotedTextFormat[] = "\"%s\"";
constexpr char kTrueText[] = "true";
constexpr char kFalseText[] = "false";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
static const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

// NTSC-U/C: 0x00505c78, PAL: 0x00544b08
static Dbg &operator<<(Dbg &sink, const std::list<Drawable *> &draws) {
    sink.Print("(size:");
    sink.Format(kSizeFormat, draws.size());
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

// NTSC-U/C: 0x00505fe8, PAL: 0x00544e78
//
// Each entry is written as the referenced object's name including its terminator, so a reader has
// to resolve the names through Rnd::TheManager. An empty entry writes one zero byte.
static Stream &operator<<(Stream &stream, const std::list<Drawable *> &draws) {
    int nCount = draws.size();
    stream.WriteLE(&nCount, sizeof(nCount));

    for (std::list<Drawable *>::const_iterator it = draws.begin(); it != draws.end(); ++it) {
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

// NTSC-U/C: 0x005062c0, PAL: 0x00545150
static Stream &operator>>(Stream &stream, std::list<Drawable *> &draws) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    draws.resize(nCount, nullptr);

    for (std::list<Drawable *>::iterator it = draws.begin(); it != draws.end(); ++it) {
        HxStr name(nullptr);
        stream.ReadString(name);
        Object *pObject = TheManager.Find(name);
        *it = dynamic_cast<Drawable *>(pObject);
    }
    return stream;
}

// NTSC-U/C: 0x005066f8, PAL: 0x005455b8
Drawable::Drawable() : mShowing(1), mHighlight(0) {
}

// NTSC-U/C: 0x00506590, PAL: 0x00545450
Drawable::~Drawable() {
    ReleaseDrawsRefs();
}

// NTSC-U/C: 0x00506920, PAL: 0x005457e8
void Drawable::Draw() {
    if (mShowing == 0) {
        return;
    }
    if (DrawShowing() == 0) {
        return;
    }
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end(); ++it) {
        (*it)->Draw();
    }
}

// NTSC-U/C: 0x005066d8, PAL: 0x00545598
void Drawable::SetShowing(int nShowing) {
    mShowing = nShowing;
}

// NTSC-U/C: 0x00506c88, PAL: 0x00545b50
void Drawable::SetHighlight(int nHighlight) {
    mHighlight = nHighlight;
}

// NTSC-U/C: 0x005066f0, PAL: 0x005455b0
int Drawable::DrawShowing() {
    return 1;
}

// NTSC-U/C: 0x00502d78, PAL: 0x00541ba0
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

// NTSC-U/C: 0x00503188, PAL: 0x00541fb0
void Drawable::AddDraw(Drawable *pDraw, Drawable *pBefore) {
    if (std::find(mDraws.begin(), mDraws.end(), pDraw) != mDraws.end()) {
        Rnd::TheDbg.Notify(kAlreadyInFormat, NameText(pDraw), NameText(this));
        return;
    }

    std::list<Drawable *>::iterator at = std::find(mDraws.begin(), mDraws.end(), pBefore);
    if (pDraw != nullptr) {
        pDraw->AddRef(this);
    }
    mDraws.insert(at, pDraw);
}

// NTSC-U/C: 0x005064e0, PAL: 0x005453a0
void Drawable::AddDraw(Drawable *pDraw) {
    AddDraw(pDraw, mDraws.empty() ? nullptr : mDraws.front());
}

// NTSC-U/C: 0x00506528, PAL: 0x005453e8
Drawable *Drawable::Find(const HxStr &name) {
    return dynamic_cast<Drawable *>(TheManager.Find(name));
}

// NTSC-U/C: 0x00503360, PAL: 0x00542188
void Drawable::RemoveDraw(Drawable *pDraw) {
    if (std::find(mDraws.begin(), mDraws.end(), pDraw) == mDraws.end()) {
        return;
    }
    if (pDraw != nullptr) {
        pDraw->RemoveRef(this);
    }
    mDraws.remove(pDraw);
}

// NTSC-U/C: 0x00503420, PAL: 0x00542248
void Drawable::RemoveAllDraws() {
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end();) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
        it = mDraws.erase(it);
    }
}

// NTSC-U/C: 0x005034b8, PAL: 0x005422e0
void Drawable::MoveDraw(Drawable *pDraw, int nSteps) {
    std::list<Drawable *>::iterator it = std::find(mDraws.begin(), mDraws.end(), pDraw);
    if (it == mDraws.end()) {
        return;
    }

    std::list<Drawable *>::iterator pos = it;
    if (nSteps > 0) {
        ++pos;
        while (pos != mDraws.end() && --nSteps != -1) {
            ++pos;
        }
    } else {
        while (pos != mDraws.begin() && ++nSteps != 1) {
            --pos;
        }
    }
    mDraws.splice(pos, mDraws, it);
}

// NTSC-U/C: 0x00506ba8, PAL: 0x00545a70
void Drawable::ReleaseDrawsRefs() {
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end(); ++it) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
    }
}

// NTSC-U/C: 0x00506c18, PAL: 0x00545ae0
void Drawable::AcquireDrawsRefs() {
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end(); ++it) {
        if (*it != nullptr) {
            (*it)->AddRef(this);
        }
    }
}

// NTSC-U/C: 0x005069a8, PAL: 0x00545870
void Drawable::DumpText(Dbg &sink) {
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

// NTSC-U/C: 0x00506b28, PAL: 0x005459f0
void Drawable::Save(Stream &stream) {
    int nRevision = kDrawableRevision;
    stream.WriteLE(&nRevision, sizeof(nRevision));

    const char cShowing = static_cast<char>(mShowing);
    stream.Write(&cShowing, sizeof(cShowing));

    stream << mDraws;
}

// NTSC-U/C: 0x00503020, PAL: 0x00541e48
void Drawable::Load(Stream &stream) {
    int nRevision = 0;
    stream.ReadLE(&nRevision, sizeof(nRevision));
    if (nRevision > kDrawableRevision) {
        Rnd::TheDbg.Notify("Can't load new Drawable\n");
        if (Rnd::TheDbg.mAbortProc != nullptr) {
            Rnd::TheDbg.mAbortProc();
        } else {
            throw; // With no handler the binary rethrows the exception in flight.
        }
    }

    ReleaseDrawsRefs();

    char cShowing = 0;
    stream.Read(&cShowing, sizeof(cShowing));
    mShowing = cShowing != 0 ? 1 : 0;

    stream >> mDraws;
    AcquireDrawsRefs();
}

// NTSC-U/C: 0x00506a88, PAL: 0x00545950
void Drawable::Copy(const Object *pSource, unsigned nFlags) {
    const Drawable *pSourceDrawable = dynamic_cast<const Drawable *>(pSource);

    ReleaseDrawsRefs();
    mShowing = pSourceDrawable->mShowing; // Yes, the binary reads this without a null check.
    if ((nFlags & kCopyChildLists) != 0) {
        mDraws = pSourceDrawable->mDraws;
    }
    AcquireDrawsRefs();
}

// NTSC-U/C: 0x00502e40, PAL: 0x00541c68
void Drawable::Replace(Object *pFrom, Object *pTo) {
    for (std::list<Drawable *>::iterator it = mDraws.begin(); it != mDraws.end();) {
        if (*it == pTo) {
            Rnd::TheDbg.Notify(kAlreadyInFormat, NameText(pTo), NameText(this));
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
