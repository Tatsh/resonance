#include "rnd/collideable.h"

#include <algorithm>
#include <list>

#include "os/dbg.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

// The only revision this build writes, and the highest it accepts.
constexpr int kCollideableRevision = 0;

constexpr char kAlreadyInFormat[] = "%s already in %s\n";
constexpr char kCountFormat[] = "%d";
constexpr char kSizeFormat[] = "%u";
constexpr char kQuotedTextFormat[] = "\"%s\"";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
static const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

// NTSC-U/C: 0x00501c48, PAL: 0x00540a20
static Dbg &operator<<(Dbg &sink, const std::list<Collideable *> &collides) {
    sink.Print("(size:");
    sink.Format(kSizeFormat, collides.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<Collideable *>::const_iterator it = collides.begin(); it != collides.end();
         ++it) {
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

// NTSC-U/C: 0x00501db0, PAL: 0x00540b88
//
// Each entry is written as the referenced object's name including its terminator. A reader has to
// resolve the names through Rnd::TheManager. An empty entry writes one zero byte.
static Stream &operator<<(Stream &stream, const std::list<Collideable *> &collides) {
    int nCount = collides.size();
    stream.WriteLE(&nCount, sizeof(nCount));

    for (std::list<Collideable *>::const_iterator it = collides.begin(); it != collides.end();
         ++it) {
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

// NTSC-U/C: 0x00502088, PAL: 0x00540e60
static Stream &operator>>(Stream &stream, std::list<Collideable *> &collides) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));
    collides.resize(nCount, nullptr);

    for (std::list<Collideable *>::iterator it = collides.begin(); it != collides.end(); ++it) {
        HxStr name(nullptr);
        stream.ReadString(name);
        Object *pObject = TheManager.Find(name);
        *it = dynamic_cast<Collideable *>(pObject);
    }
    return stream;
}

Collideable::Collideable() {
}

Collideable::~Collideable() {
    ReleaseCollidesRefs();
}

Collideable *Collideable::Parent() {
    for (std::list<Object *>::iterator it = mRefs.begin(); it != mRefs.end(); ++it) {
        Collideable *pCandidate = dynamic_cast<Collideable *>(*it);
        if (pCandidate == nullptr) {
            continue;
        }
        if (std::find(pCandidate->mCollides.begin(), pCandidate->mCollides.end(), this) !=
            pCandidate->mCollides.end()) {
            return pCandidate;
        }
    }
    return nullptr;
}

void Collideable::AddCollide(Collideable *pCollide) {
    if (std::find(mCollides.begin(), mCollides.end(), pCollide) != mCollides.end()) {
        Rnd::TheDbg.Notify(kAlreadyInFormat, NameText(pCollide), NameText(this));
        return;
    }

    if (pCollide != nullptr) {
        pCollide->AddRef(this);
    }
    mCollides.push_back(pCollide);
}

void Collideable::RemoveCollide(Collideable *pCollide) {
    if (std::find(mCollides.begin(), mCollides.end(), pCollide) == mCollides.end()) {
        return;
    }

    if (pCollide != nullptr) {
        pCollide->RemoveRef(this);
    }
    mCollides.remove(pCollide);
}

void Collideable::FindCollisions(const Segment &ray, std::list<Collision> &collisions) {
    for (std::list<Collideable *>::iterator it = mCollides.begin(); it != mCollides.end(); ++it) {
        (*it)->FindCollisions(ray, collisions);
    }
}

void Collideable::FindCollisions(const Vector2 &point, std::list<Collision> &collisions) {
    for (std::list<Collideable *>::iterator it = mCollides.begin(); it != mCollides.end(); ++it) {
        (*it)->FindCollisions(point, collisions);
    }
}

void Collideable::ReleaseCollidesRefs() {
    for (std::list<Collideable *>::iterator it = mCollides.begin(); it != mCollides.end(); ++it) {
        if (*it != nullptr) {
            (*it)->RemoveRef(this);
        }
    }
}

void Collideable::AcquireCollidesRefs() {
    for (std::list<Collideable *>::iterator it = mCollides.begin(); it != mCollides.end(); ++it) {
        if (*it != nullptr) {
            (*it)->AddRef(this);
        }
    }
}

void Collideable::DumpText(Dbg &sink) {
    if (sink.mDumpLevel <= 0) {
        return;
    }
    sink.Print("[Collideable]\n");
    sink.Print("collides:");
    sink << mCollides;
    sink.Print("\n");
}

void Collideable::Save(Stream &stream) {
    int nRevision = kCollideableRevision;
    stream.WriteLE(&nRevision, sizeof(nRevision));

    stream << mCollides;
}

void Collideable::Load(Stream &stream) {
    int nRevision = 0;
    stream.ReadLE(&nRevision, sizeof(nRevision));
    if (nRevision > kCollideableRevision) {
        Rnd::TheDbg.Notify("Can't load new Collideable\n");
        if (Rnd::TheDbg.mAbortProc != nullptr) {
            Rnd::TheDbg.mAbortProc();
        } else {
            throw; // With no handler the binary rethrows the exception in flight.
        }
    }

    ReleaseCollidesRefs();

    stream >> mCollides;
    AcquireCollidesRefs();
}

void Collideable::Copy(const Object *pSource, unsigned nFlags) {
    const Collideable *pSourceCollideable = dynamic_cast<const Collideable *>(pSource);

    ReleaseCollidesRefs();
    if ((nFlags & kCopyChildLists) != 0) {
        mCollides = pSourceCollideable->mCollides;
    }
    AcquireCollidesRefs();
}

void Collideable::Replace(Object *pFrom, Object *pTo) {
    for (std::list<Collideable *>::iterator it = mCollides.begin(); it != mCollides.end();) {
        if (*it == pTo) {
            Rnd::TheDbg.Notify(kAlreadyInFormat, NameText(pTo), NameText(this));
        }

        if (*it == pFrom) {
            if (pFrom != nullptr) {
                pFrom->RemoveRef(this);
            }
            if (*it != nullptr) {
                *it = dynamic_cast<Collideable *>(pTo);
            }
            if (*it != nullptr) {
                (*it)->AddRef(this);
            }
        }

        if (*it == nullptr) {
            it = mCollides.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace Rnd
