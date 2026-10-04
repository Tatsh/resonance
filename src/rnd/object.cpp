#include "rnd/object.h"

#include <list>

#include "os/dbg.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/stream.h"

namespace Rnd {

constexpr char kAlreadyExistsFormat[] = "%s already exists\n";
constexpr char kQuotedTextFormat[] = "\"%s\"";
constexpr char kCountFormat[] = "%u";
constexpr char kIndexFormat[] = "%d";
constexpr char kTrueText[] = "true";
constexpr char kFalseText[] = "false";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
static const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

// NTSC-U/C: 0x0053fa40, PAL: 0x0057f6f8
static Dbg &operator<<(Dbg &sink, const std::list<Object *> &refs) {
    sink.Print("(size:");
    sink.Format(kCountFormat, refs.size());
    sink.Print(")");

    int nIndex = 0;
    for (std::list<Object *>::const_iterator it = refs.begin(); it != refs.end(); ++it) {
        sink.Print("\n");
        sink.Format(kIndexFormat, nIndex);
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

void (*g_pfnNameChanged)(Object *pObject);

Object::Object() : mName(nullptr) {
}

Object::Object(const HxStr &name) : mName(name), mInternal(0), mMerge(1), mDeleting(0) {
    if (TheManager.Find(name) != nullptr) {
        Rnd::TheDbg.Notify(kAlreadyExistsFormat, name.mStr != nullptr ? name.mStr : "");
        if (Rnd::TheDbg.mAbortProc != nullptr) {
            Rnd::TheDbg.mAbortProc();
        } else {
            throw; // With no handler the binary rethrows the exception in flight.
        }
    }

    // The duplicate above is overwritten rather than preserved.
    TheManager.mObjects[mName] = this;
}

Object::~Object() {
    if (g_pfnNameChanged != nullptr) {
        g_pfnNameChanged(this);
    }
    TheManager.mObjects.erase(mName);
}

void Object::SetName(const HxStr &name) {
    if (mName == name) {
        return;
    }
    if (TheManager.Find(name) != nullptr) {
        Rnd::TheDbg.Notify(kAlreadyExistsFormat, name.mStr != nullptr ? name.mStr : "");
        return;
    }

    if (g_pfnNameChanged != nullptr) {
        g_pfnNameChanged(this);
    }
    TheManager.mObjects.erase(mName);
    mName = name;
    TheManager.mObjects[mName] = this;
}

void Object::AddRef(Object *pReferrer) {
    if (pReferrer == this) {
        return;
    }
    mRefs.push_front(pReferrer);
}

void Object::RemoveRef(Object *pReferrer) {
    if (mDeleting != 0) {
        return;
    }
    for (std::list<Object *>::iterator it = mRefs.begin(); it != mRefs.end(); ++it) {
        if (*it == pReferrer) {
            mRefs.erase(it);
            return;
        }
    }
}

void Object::ReleaseAllRefs() {
    mDeleting = 1;
    mRefs.unique();
    for (std::list<Object *>::iterator it = mRefs.begin(); it != mRefs.end(); ++it) {
        (*it)->Replace(this, nullptr);
    }
    mRefs.clear();
}

void Object::DumpText(Dbg &sink) {
    sink.Print("[Object]\n");
    sink.Print("name:");
    sink.Format(kQuotedTextFormat, NameText(this));
    sink.Print(" class:");
    sink.Format(kQuotedTextFormat, ClassName().mStr != nullptr ? ClassName().mStr : "");
    sink.Print(" internal:");
    sink.Print((mInternal != 0 ? kTrueText : kFalseText));
    sink.Print(" merge:");
    sink.Print((mMerge != 0 ? kTrueText : kFalseText));
    sink.Print("\n");

    if (sink.mDumpLevel > 0) {
        sink.Print("references:");
        sink << mRefs;
        sink.Print("\n");
    }
}

} // namespace Rnd
