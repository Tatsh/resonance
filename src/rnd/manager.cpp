#include "rnd/manager.h"

#include <iterator>
#include <list>
#include <map>
#include <utility>

#include "app/longop.h"
#include "os/dbg.h"
#include "os/hxstr.h"
#include "profile/profiler.h"
#include "rnd/arena.h"
#include "rnd/blur.h"
#include "rnd/button.h"
#include "rnd/cam.h"
#include "rnd/collectchildren.h"
#include "rnd/environ.h"
#include "rnd/filestream.h"
#include "rnd/font.h"
#include "rnd/generator.h"
#include "rnd/light.h"
#include "rnd/lightanim.h"
#include "rnd/mat.h"
#include "rnd/matanim.h"
#include "rnd/mesh.h"
#include "rnd/meshanim.h"
#include "rnd/movie.h"
#include "rnd/multimesh.h"
#include "rnd/object.h"
#include "rnd/particlesys.h"
#include "rnd/particlesysanim.h"
#include "rnd/stream.h"
#include "rnd/string.h"
#include "rnd/tex.h"
#include "rnd/text.h"
#include "rnd/transanim.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace Rnd {

namespace {

// Dump level at which the objects the renderer created itself are written as well.
constexpr int kInternalObjectDumpLevel = 2;

// The FileStream modes LoadFile() and SaveFile() open with.
constexpr int kOpenForReading = 0;
constexpr int kOpenForWriting = 1;

// Profile timer records Init() labels, from the first to one past the last.
constexpr int kFirstRendererTimer = 14;

// Read() calls the long-operation draw hook once per this many table entries.
constexpr int kEntriesPerLoadFrame = 8;

// File versions that introduced the per-entry merge byte and the record end marker.
constexpr int kMergeFlagVersion = 1;
constexpr int kRecordMarkerVersion = 2;

// File versions below which RemapLegacyClassName() applies each rewrite.
constexpr int kMixInRenameVersion = 3;
constexpr int kSpriteRenameVersion = 4;
constexpr int kMovieRenameVersion = 5;
constexpr int kGeneratorRenameVersion = 6;

// The two byte values of the record end marker, in stream order: ad de ad de.
constexpr unsigned char kMarkerLowByte = 0xad;
constexpr unsigned char kMarkerHighByte = 0xde;

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty string.
inline const char *NameText(const HxStr &name) {
    return name.mStr != nullptr ? name.mStr : g_szEmptyString;
}

// Consumes bytes through the end of the next record end marker.
inline void SkipPastRecordMarker(Stream &stream) {
    unsigned char ch;
    stream.Read(&ch, sizeof(ch));
    for (;;) {
        if (ch == kMarkerLowByte) {
            stream.Read(&ch, sizeof(ch));
            if (ch != kMarkerHighByte) {
                continue;
            }
            stream.Read(&ch, sizeof(ch));
            if (ch == kMarkerLowByte) {
                stream.Read(&ch, sizeof(ch));
                if (ch == kMarkerHighByte) {
                    return;
                }
                continue;
            }
        }
        stream.Read(&ch, sizeof(ch));
    }
}

// NTSC-U/C: 0x0051f798, PAL: 0x0055fcc0
// Writes the class registry as its entry count and then one indented line per entry.
// The factory is reported as a truth value rather than as an address, which is what the two
// literals "true" and "false" at 0x00826e20 and 0x00826e28 are for. Only DumpText() calls it.
Dbg *DumpRegisteredClasses(Dbg *pSink, const std::map<HxStr, ClassFactory> &classes) {
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

// NTSC-U/C: 0x0051f938, PAL: 0x0055fe60
template void MoveClassToFront<Font>(std::list<Object *> &objects, Font *);

// NTSC-U/C: 0x0051fa80, PAL: 0x0055ffa8
template void MoveClassToFront<Mat>(std::list<Object *> &objects, Mat *);

// NTSC-U/C: 0x0051fbc8, PAL: 0x005600f0
template void MoveClassToFront<Tex>(std::list<Object *> &objects, Tex *);

// NTSC-U/C: 0x0051fd10, PAL: 0x00560238
template void MoveClassToFront<TransAnim>(std::list<Object *> &objects, TransAnim *);

// The table version Write() stores, and the marker it writes after every object record.
constexpr int kManagerFileVersion = 6;
constexpr unsigned kRecordEndMarker = 0xdeaddead;

// A name written with its terminator. An empty HxStr stores a null buffer, and the binary writes
// the program-wide empty string in its place.
inline Stream &WriteTerminatedName(Stream &stream, const HxStr &name) {
    return stream.Write(name.mStr != nullptr ? name.mStr : g_szEmptyString, name.mLen + 1);
}

} // namespace

// NTSC-U/C: 0x0089df90, PAL: 0x008e2fd0
int g_nRndManagerFileVersion;

// NTSC-U/C: 0x00719888, PAL: 0x0075d788
int g_nRndManagerLoadFrameCounter;

Manager::Manager() {
}

void Manager::Init() {
    ResetFrameTimer();

    const char *timerNames[] = {"callback", "anim", "updateworldxfm", "draw", "swap", "frame"};
    for (int i = kFirstRendererTimer;
         i < kFirstRendererTimer + static_cast<int>(sizeof(timerNames) / sizeof(timerNames[0]));
         ++i) {
        g_profileTimers[i].mName = HxStr(timerNames[i - kFirstRendererTimer]);
    }

    Blur::Init();
    RegisterCamClass();
    RegisterMeshClass();
    RegisterTextClass();
    RegisterFontClass();
    RegisterLightClass();
    RegisterEnvironClass();
    RegisterMatClass();
    RegisterTexClass();
    RegisterMovieClass();
    RegisterTransAnimClass();
    RegisterLightAnimClass();
    RegisterMeshAnimClass();
    RegisterMatAnimClass();
    View::Init();
    RegisterTunnelClass();
    String::Init();
    Generator::Init();
    RegisterButtonClass();
    Arena::Init();
    RegisterParticleSysClass();
    RegisterParticleSysAnimClass();
    RegisterMultiMeshClass();
}

void Manager::RegisterClass(const HxStr &name, ClassFactory pfnCreate) {
    mClasses[name] = pfnCreate;
}

Object *Manager::Create(const HxStr &className, const HxStr &objectName) {
    const auto it = mClasses.find(className);
    if (it != mClasses.end()) {
        return it->second(objectName);
    }
    Rnd::TheDbg.Notify("Class %s is unregistered\n", NameText(className));
    return nullptr;
}

Object *Manager::Find(const HxStr &name) {
    const auto it = mObjects.find(name);
    return it != mObjects.end() ? it->second : nullptr;
}

void Manager::RemapLegacyClassName(HxStr &name) {
    if (g_nRndManagerFileVersion < kMixInRenameVersion) {
        if (name == "AnimObject") {
            name = "Animatable";
        } else if (name == "DrawObject") {
            name = "Drawable";
        } else if (name == "CollideObject") {
            name = "Collideable";
        } else if (name == "TransObject") {
            name = "Transformable";
        }
    }
    if (g_nRndManagerFileVersion < kSpriteRenameVersion && name == "DrawRect") {
        name = "Sprite";
    }
    if (g_nRndManagerFileVersion < kMovieRenameVersion && name == "TexMovie") {
        name = "Movie";
    }
    if (g_nRndManagerFileVersion < kGeneratorRenameVersion && name == "MeshGenerator") {
        name = "Generator";
    }
}

void Manager::Read(Stream &stream) {
    stream.ReadLE(&g_nRndManagerFileVersion, sizeof(g_nRndManagerFileVersion));
    if (g_nRndManagerFileVersion > kManagerFileVersion) {
        Rnd::TheDbg.Notify("Can't load new Manager\n");
        return;
    }

    int nCount;
    stream.ReadLE(&nCount, sizeof(nCount));
    mLoaded.clear();
    mMergeObjects.clear();

    // Each object paired with its mMerge as this pass found it.
    std::list<std::pair<Object *, int>> entries;
    for (; nCount > 0; --nCount) {
        HxStr className;
        HxStr objectName;
        stream.ReadString(className).ReadString(objectName);
        RemapLegacyClassName(className);

        if (++g_nRndManagerLoadFrameCounter == kEntriesPerLoadFrame) {
            g_nRndManagerLoadFrameCounter = 0;
            if (g_pfnLongOperationDrawProc != nullptr) {
                g_pfnLongOperationDrawProc();
            }
        }

        int bMerge = 1;
        if (g_nRndManagerFileVersion >= kMergeFlagVersion) {
            unsigned char chMerge;
            stream.Read(&chMerge, sizeof(chMerge));
            bMerge = chMerge != 0;
        }

        bool bCreated = false;
        Object *pObject = Find(objectName);
        if (pObject == nullptr) {
            pObject = Create(className, objectName);
            if (pObject == nullptr) {
                Rnd::TheDbg.Notify("Failed to create object %s of class %s\n",
                                   NameText(objectName),
                                   NameText(className));
                if (Rnd::TheDbg.mAbortProc != nullptr) {
                    Rnd::TheDbg.mAbortProc();
                } else {
                    throw; // With no handler the binary rethrows the exception in flight.
                }
                continue;
            }
            bCreated = true;
        }

        if (bCreated) {
            mLoaded.push_back(pObject);
        } else {
            bool bAccepted = false;
            if (pObject->mInternal == 0) {
                if (pObject->ClassName() == className) {
                    bAccepted = true;
                } else if (pObject->ClassName() == "View") {
                    bAccepted = className == "Animatable" || className == "Transformable" ||
                                className == "Drawable" || className == "Collideable";
                }
            }
            if (!bAccepted) {
                Rnd::TheDbg.Notify("Can't merge object %s\n", NameText(pObject->mName));
                return;
            }
            if (pObject->mMerge != 0) {
                mMergeObjects.push_back(pObject);
            }
        }

        entries.push_back(std::make_pair(pObject, pObject->mMerge));
        if (pObject->mMerge != 0) {
            pObject->mMerge = bMerge;
        }
    }

    for (const auto &entry : entries) {
        if (g_pfnLongOperationDrawProc != nullptr) {
            g_pfnLongOperationDrawProc();
        }
        if (entry.second != 0) {
            entry.first->Load(stream);
            if (g_nRndManagerFileVersion >= kRecordMarkerVersion) {
                SkipPastRecordMarker(stream);
            }
        } else if (g_nRndManagerFileVersion >= kRecordMarkerVersion) {
            SkipPastRecordMarker(stream);
        } else {
            Object *pTemp = Create(entry.first->ClassName(), HxStr("__temp__"));
            pTemp->Load(stream); // Yes, the binary calls Load() before the null test.
            if (pTemp != nullptr) {
                delete pTemp;
            }
        }
    }
}

void Manager::LoadFile(const HxStr &path) {
    FileStream stream(path, kOpenForReading);
    if (stream.Fail()) {
        Rnd::TheDbg.Notify("Could not open file: %s\n", NameText(path));
        mLoaded.clear();
        mMergeObjects.clear();
        return;
    }
    Read(stream);
}

Manager::~Manager() {
}

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
    stream.WriteLE(&nVersion, sizeof(nVersion)).WriteLE(&nCount, sizeof(nCount));
    for (Object *pObject : objects) {
        const char chMerge = static_cast<char>(pObject->mMerge);
        WriteTerminatedName(WriteTerminatedName(stream, pObject->ClassName()), pObject->mName)
            .Write(&chMerge, sizeof(chMerge));
    }
    for (Object *pObject : objects) {
        pObject->Save(stream);
        const unsigned nMarker = kRecordEndMarker;
        stream.WriteLE(&nMarker, sizeof(nMarker));
    }
}

bool Manager::Contains(const Object *pObject) {
    for (const auto &entry : mObjects) {
        if (entry.second == pObject) {
            return true;
        }
    }
    return false;
}

void Manager::SaveFile(const HxStr &path) {
    FileStream stream(path, kOpenForWriting);
    if (stream.Fail()) {
        Rnd::TheDbg.Notify("Could not open file: %s\n",
                           path.mStr != nullptr ? path.mStr : g_szEmptyString);
        return;
    }
    Write(stream);
}

void Manager::DumpText(Dbg &sink) {
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
            clones.push_back(TheManager.ResolveAndLinkObject(*it, prefix, nFlags, 0, 0));
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

Manager TheManager;

} // namespace Rnd
