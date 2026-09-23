#include "rnd/manager.h"

#include <iterator>
#include <list>
#include <map>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/collectchildren.h"
#include "rnd/filestream.h"
#include "rnd/font.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rnd/transanim.h"

namespace Rnd {

namespace {

// Dump level at which the objects the renderer created itself are written as well.
constexpr int kInternalObjectDumpLevel = 2;

// The FileStream mode SaveFile() opens with.
constexpr int kOpenForWriting = 1;

// 0x0051f798
// Writes the class registry as its entry count and then one indented line per entry.
// The factory is reported as a truth value rather than as an address, which is what the two
// literals "true" and "false" at 0x00826e20 and 0x00826e28 are for. Only DumpText() calls it.
FailSink *DumpRegisteredClasses(FailSink *pSink, const std::map<HxStr, ClassFactory> &classes) {
    pSink->Print("(size:")->Format("%u", classes.size())->Print(")");

    for (const auto &entry : classes) {
        pSink->Print("\n\t")
            ->Print("key:")
            ->Format("\"%s\"", entry.first.mStr != nullptr ? entry.first.mStr : g_szEmptyString)
            ->Print(" value:")
            ->Print(entry.second != nullptr ? "true" : "false");
    }

    return pSink;
}

// Move every object of class T to the front of objects. Each match is spliced to the front in
// turn, so the matches end up in reverse order. The unused null pointer selects T, the idiom a
// compiler without explicit template arguments at the call needs; Write() passes zero in a1.
template <class T>
void MoveClassToFront(std::list<Object *> &objects, T *) {
    for (auto it = objects.begin(); it != objects.end();) {
        const auto next = std::next(it);
        if (*it != nullptr && dynamic_cast<T *>(*it) != nullptr) {
            objects.splice(objects.begin(), objects, it);
        }
        it = next;
    }
}

// 0x0051f938
template void MoveClassToFront<Font>(std::list<Object *> &objects, Font *);

// 0x0051fa80
template void MoveClassToFront<Mat>(std::list<Object *> &objects, Mat *);

// 0x0051fbc8
template void MoveClassToFront<Tex>(std::list<Object *> &objects, Tex *);

// 0x0051fd10
template void MoveClassToFront<TransAnim>(std::list<Object *> &objects, TransAnim *);

// The table version Write() stores, and the marker it writes after every object record.
constexpr int kManagerFileVersion = 6;
constexpr unsigned kRecordEndMarker = 0xdeaddead;

// A name written with its terminator. An empty HxStr stores a null buffer, and the binary writes
// the program-wide empty string in its place.
inline Stream &WriteTerminatedName(Stream &stream, const HxStr &name) {
    return stream.WriteBytes(name.mStr != nullptr ? name.mStr : g_szEmptyString, name.mLen + 1);
}

} // namespace

// 0x005200c0
Manager::Manager() {
}

// 0x00520348
Manager::~Manager() {
}

// 0x0051aff8
void Manager::Write(Stream &stream) {
    std::list<Object *> objects;
    for (const auto &entry : mObjects) {
        if (entry.second->mInternal == 0) {
            objects.push_back(entry.second);
        }
    }
    MoveClassToFront(objects, static_cast<Font *>(nullptr));
    MoveClassToFront(objects, static_cast<Mat *>(nullptr));
    MoveClassToFront(objects, static_cast<Tex *>(nullptr));
    MoveClassToFront(objects, static_cast<TransAnim *>(nullptr));

    const int nVersion = kManagerFileVersion;
    const int nCount = static_cast<int>(objects.size());
    stream.Write(&nVersion, sizeof(nVersion)).Write(&nCount, sizeof(nCount));
    for (Object *pObject : objects) {
        const char chMerge = static_cast<char>(pObject->mMerge);
        WriteTerminatedName(WriteTerminatedName(stream, pObject->ClassName()), pObject->mName)
            .WriteBytes(&chMerge, sizeof(chMerge));
    }
    for (Object *pObject : objects) {
        pObject->Save(stream);
        const unsigned nMarker = kRecordEndMarker;
        stream.Write(&nMarker, sizeof(nMarker));
    }
}

// 0x005199d8
bool Manager::Contains(const Object *pObject) {
    for (const auto &entry : mObjects) {
        if (entry.second == pObject) {
            return true;
        }
    }
    return false;
}

// 0x005204e8
void Manager::SaveFile(const HxStr &path) {
    FileStream stream(path, kOpenForWriting);
    if (stream.Fail()) {
        g_failSink.Report("Could not open file: %s\n",
                          path.mStr != nullptr ? path.mStr : g_szEmptyString);
        return;
    }
    Write(stream);
}

// 0x0051ad98
void Manager::DumpText(FailSink &sink) {
    sink.Print("[Manager]\n");
    DumpRegisteredClasses(sink.Print("registeredClasses:"), mClasses)->Print("\n");

    sink.Print("objects:\n\n");
    for (const auto &entry : mObjects) {
        if (entry.second->mInternal == 0) {
            entry.second->DumpText(sink);
            sink.Print("\n");
        }
    }

    if (sink.mDumpLevel < kInternalObjectDumpLevel) {
        return;
    }

    sink.Print("internalObjects:\n\n");
    for (const auto &entry : mObjects) {
        if (entry.second->mInternal != 0) {
            entry.second->DumpText(sink);
            sink.Print("\n");
        }
    }
}

// 0x0051a428
Object *Manager::ResolveAndLinkObject(
    Object *pSource, const HxStr &prefix, unsigned nFlags, int bRecurse, int bLink) {
    const HxStr &className = pSource->ClassName();
    Object *pClone;
    {
        HxStr name = prefix + pSource->mName;
        pClone = Create(className, name);
    }
    if (pClone == nullptr) {
        return nullptr;
    }
    pClone->Copy(pSource, nFlags);
    mLoaded.clear();
    mLoaded.push_back(pClone);
    if (bRecurse == 0 && bLink == 0) {
        return pClone;
    }

    Animatable *pAnimatable = dynamic_cast<Animatable *>(pSource);
    Collideable *pCollideable = dynamic_cast<Collideable *>(pSource);
    Drawable *pDrawable = dynamic_cast<Drawable *>(pSource);
    Transformable *pTransformable = dynamic_cast<Transformable *>(pSource);

    if (bRecurse != 0) {
        std::list<Object *> sources;
        CollectChildren(sources, pAnimatable);
        CollectChildren(sources, pCollideable);
        CollectChildren(sources, pDrawable);
        CollectChildren(sources, pTransformable);
        sources.sort();
        sources.unique();

        std::list<Object *> clones;
        for (std::list<Object *>::iterator it = sources.begin(); it != sources.end(); ++it) {
            clones.push_back(g_manager.ResolveAndLinkObject(*it, prefix, nFlags, 0, 0));
        }
        sources.push_back(pSource);
        clones.push_back(pClone);

        // Every clone's references to a source object are redirected to that object's clone.
        std::list<Object *>::iterator clone = clones.begin();
        for (std::list<Object *>::iterator source = sources.begin(); source != sources.end();
             ++source, ++clone) {
            for (std::list<Object *>::iterator other = clones.begin(); other != clones.end();
                 ++other) {
                if (other != clone) {
                    (*other)->Replace(*source, *clone);
                }
            }
        }

        mLoaded.clear();
        mLoaded.splice(mLoaded.end(), clones);
    }

    if (bLink != 0) {
        if (pTransformable != nullptr) {
            Transformable *pParent = pTransformable->Parent();
            if (pParent != nullptr) {
                pParent->AddTrans(dynamic_cast<Transformable *>(pClone));
            }
        }
        if (pCollideable != nullptr) {
            Collideable *pParent = pCollideable->Parent();
            if (pParent != nullptr) {
                pParent->AddCollide(dynamic_cast<Collideable *>(pClone));
            }
        }
        if (pDrawable != nullptr) {
            Drawable *pParent = pDrawable->Parent();
            if (pParent != nullptr) {
                pParent->AddDraw(dynamic_cast<Drawable *>(pClone), nullptr);
            }
        }
        if (pAnimatable != nullptr) {
            Animatable *pParent = pAnimatable->Parent();
            if (pParent != nullptr) {
                pParent->AddAnim(dynamic_cast<Animatable *>(pClone));
            }
        }
    }
    return pClone;
}

// 0x0051bf70
void Manager::DeleteLoadedObjects() {
    for (;;) {
        auto entry = mObjects.begin();
        while (entry != mObjects.end() && entry->second->mInternal != 0) {
            ++entry;
        }
        if (entry == mObjects.end()) {
            return;
        }
        // An entry whose object is null spins here rather than making progress, because the
        // destroy is what would have erased it.
        delete entry->second;
    }
}

} // namespace Rnd
