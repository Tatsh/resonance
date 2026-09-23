#include "rnd/object.h"

#include <list>

#include "os/failsink.h"
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

// 0x0053fa40
static FailSink &operator<<(FailSink &sink, const std::list<Object *> &refs) {
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

// 0x0053fc08
//
// mInternal, mMerge, and mDeleting are not written here. The named constructor below does set all
// three.
Object::Object() : mName(nullptr) {
}

// 0x0053e0d8
Object::Object(const HxStr &name) : mName(name), mInternal(0), mMerge(1), mDeleting(0) {
    if (g_manager.Find(name) != nullptr) {
        g_failSink.Report(kAlreadyExistsFormat, name.mStr != nullptr ? name.mStr : "");
        if (g_failSink.mAbortProc != nullptr) {
            g_failSink.mAbortProc();
        } else {
            throw; // With no handler the binary rethrows the exception in flight.
        }
    }

    // The duplicate above is overwritten rather than preserved.
    g_manager.mObjects[mName] = this;
}

// 0x0053e348
Object::~Object() {
    if (g_pfnNameChanged != nullptr) {
        g_pfnNameChanged(this);
    }
    g_manager.mObjects.erase(mName);
}

// 0x0053e400
void Object::SetName(const HxStr &name) {
    if (mName == name) {
        return;
    }
    if (g_manager.Find(name) != nullptr) {
        g_failSink.Report(kAlreadyExistsFormat, name.mStr != nullptr ? name.mStr : "");
        return;
    }

    if (g_pfnNameChanged != nullptr) {
        g_pfnNameChanged(this);
    }
    g_manager.mObjects.erase(mName);
    mName = name;
    g_manager.mObjects[mName] = this;
}

// 0x0053e720
void Object::AddRef(Object *pReferrer) {
    if (pReferrer == this) {
        return;
    }
    mRefs.push_front(pReferrer);
}

// 0x0053e7d0
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

// 0x0053fca0
void Object::ReleaseAllRefs() {
    mDeleting = 1;
    mRefs.unique();
    for (std::list<Object *>::iterator it = mRefs.begin(); it != mRefs.end(); ++it) {
        (*it)->Replace(this, nullptr);
    }
    mRefs.clear();
}

// 0x0053e5a8
void Object::DumpText(FailSink &sink) {
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
