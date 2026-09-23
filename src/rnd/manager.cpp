#include "rnd/manager.h"

#include <list>
#include <map>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/collectchildren.h"
#include "rnd/object.h"

namespace Rnd {

namespace {

// Dump level at which the objects the renderer created itself are written as well.
constexpr int kInternalObjectDumpLevel = 2;

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

} // namespace

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
