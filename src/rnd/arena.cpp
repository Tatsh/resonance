#include "rnd/arena.h"

#include <list>
#include <math.h>
#include <vector>

#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/animatable.h"
#include "rnd/collideable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"
#include "rnd/view.h"

namespace Rnd {

namespace {

// The only revision Save() writes.
constexpr int kArenaRevision = 4;

// Lowest revision Load() refuses.
constexpr int kArenaRejectedRevision = 5;

// First revision that stores Section::mDelta.
constexpr int kArenaSectionDeltaRevision = 1;

// First revision whose three extra base blocks are present.
constexpr int kArenaBaseBlockRevision = 2;

// First revision that stores Section::mTeleport.
constexpr int kArenaTeleportRevision = 3;

// First revision that stores Section::mSortStart.
constexpr int kArenaSortStartRevision = 4;

constexpr char kArenaTag[] = "Rnd::Arena";

// The row of a transform that stores the translation.
constexpr int kXfmTranslationRow = 3;

constexpr char kNoObject[] = "no object";
constexpr char kQuotedTextFormat[] = "\"%s\"";
constexpr char kFloatFormat[] = "%.2f";
constexpr char kCountFormat[] = "%u";
constexpr char kIndexFormat[] = "%d";
constexpr char kTrueText[] = "true";
constexpr char kFalseText[] = "false";

// An empty HxStr stores a null buffer, and the binary substitutes the program-wide empty-string
// pointer at 0x006fbd10 rather than passing null to the formatter.
const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : "";
}

void PrintObjectRef(FailSink &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format(kQuotedTextFormat, NameText(pObject));
}

void PrintBool(FailSink &sink, int nValue) {
    sink.Print(nValue != 0 ? kTrueText : kFalseText);
}

// The padding word of the vector is not written.
void PrintVector(FailSink &sink, const Vector3 &vec) {
    sink.Print("(x:");
    sink.Format(kFloatFormat, vec.x);
    sink.Print(" y:");
    sink.Format(kFloatFormat, vec.y);
    sink.Print(" z:");
    sink.Format(kFloatFormat, vec.z);
    sink.Print(")");
}

// Each reference is written as the referenced object's name including its terminator. An absent
// reference writes one zero byte, which is the empty name a reader resolves to nothing.
void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, sizeof(chTerminator));
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
}

// A byte is written for each of the two section flags even though both are stored as words.
void WriteBool(Stream &stream, int nValue) {
    const char chFlag = static_cast<char>(nValue);
    stream.Write(&chFlag, sizeof(chFlag));
}

int ReadBool(Stream &stream) {
    char chFlag = 0;
    stream.ReadBytes(&chFlag, sizeof(chFlag));
    return chFlag != 0 ? 1 : 0;
}

} // namespace

// 0x00777100
HxStr g_arenaClassName("Arena");

// 0x008e4a4c
int g_nRndArenaLoadRevision;

// 0x00777108
Arena *(*g_pfnNewArena)(const HxStr &name);

// 0x005b82b0
static FailSink &operator<<(FailSink &sink, const Arena::Section &section) {
    sink.Print("\n\tview:");
    PrintObjectRef(sink, section.mView);
    sink.Print(" frame:");
    sink.Format(kFloatFormat, section.mFrame);
    sink.Print(" loop:");
    sink.Format(kIndexFormat, section.mLoop);
    sink.Print("\n");
    sink.Print("\n\tdelta:");
    sink.Format(kFloatFormat, section.mDelta);
    sink.Print(" teleport:");
    PrintBool(sink, section.mTeleport);
    sink.Print(" sortStart:");
    PrintBool(sink, section.mSortStart);
    return sink;
}

// 0x005ba640
static FailSink &operator<<(FailSink &sink, const std::vector<Arena::Section> &sections) {
    sink.Print("(size:");
    sink.Format(kCountFormat, sections.size());
    sink.Print(")");

    for (std::vector<Arena::Section>::const_iterator it = sections.begin(); it != sections.end();
         ++it) {
        sink.Print("\n");
        sink.Format(kIndexFormat, static_cast<int>(it - sections.begin()));
        sink.Print("\t");
        sink << *it;
    }
    return sink;
}

// 0x005ba750
static Stream &operator<<(Stream &stream, const std::vector<Arena::Section> &sections) {
    const int nCount = sections.size();
    stream.Write(&nCount, sizeof(nCount));

    for (std::vector<Arena::Section>::const_iterator it = sections.begin(); it != sections.end();
         ++it) {
        WriteObjectRef(stream, it->mView);
        stream.Write(&it->mFrame, sizeof(it->mFrame));
        stream.Write(&it->mLoop, sizeof(it->mLoop));
        stream.Write(&it->mDelta, sizeof(it->mDelta));
        WriteBool(stream, it->mTeleport);
        WriteBool(stream, it->mSortStart);
    }
    return stream;
}

// 0x005b8450
static Stream &operator>>(Stream &stream, Arena::Section &section) {
    HxStr viewName(nullptr);
    stream.ReadString(viewName);
    section.mView = dynamic_cast<View *>(g_manager.Find(viewName));

    stream.Read(&section.mFrame, sizeof(section.mFrame));
    stream.Read(&section.mLoop, sizeof(section.mLoop));

    if (g_nRndArenaLoadRevision >= kArenaSectionDeltaRevision) {
        stream.Read(&section.mDelta, sizeof(section.mDelta));
    }
    if (g_nRndArenaLoadRevision >= kArenaTeleportRevision) {
        section.mTeleport = ReadBool(stream);
    }
    if (g_nRndArenaLoadRevision >= kArenaSortStartRevision) {
        section.mSortStart = ReadBool(stream);
    }
    return stream;
}

// 0x005ba908
static Stream &operator>>(Stream &stream, std::vector<Arena::Section> &sections) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));

    sections.resize(nCount, Arena::Section());

    for (std::vector<Arena::Section>::iterator it = sections.begin(); it != sections.end(); ++it) {
        stream >> *it;
    }
    return stream;
}

// 0x005bb9d8
const HxStr &Arena::ClassName() const {
    return g_arenaClassName;
}

// 0x005bb9e8
Vector3 &Arena::LoopDist() {
    return mLoopDist;
}

// 0x005b5f08
void Arena::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    Transformable::DumpText(sink);
    Drawable::DumpText(sink);
    Collideable::DumpText(sink);

    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[Arena]\n");
    sink.Print("loopDist:");
    PrintVector(sink, mLoopDist);
    sink.Print(" loopFrames:");
    sink.Format(kFloatFormat, mLoopFrames);
    sink.Print("\n");
    sink.Print("sections:");
    sink << mSections;
    sink.Print("\n");
}

// 0x005b60c0
//
// The base blocks are written in a different order from the one DumpText() uses, and mDrawOrder
// is not written at all, because AddInstancesToHitList() rebuilds it from the section count.
void Arena::Save(Stream &stream) {
    const int nRevision = kArenaRevision;
    stream.Write(&nRevision, sizeof(nRevision));

    Animatable::Save(stream);
    Collideable::Save(stream);
    Drawable::Save(stream);
    Transformable::Save(stream);

    stream.Write(&mLoopDist.x, sizeof(mLoopDist.x));
    stream.Write(&mLoopDist.y, sizeof(mLoopDist.y));
    stream.Write(&mLoopDist.z, sizeof(mLoopDist.z));
    stream.Write(&mLoopFrames, sizeof(mLoopFrames));

    stream << mSections;
}

// 0x005b61e8
void Arena::Load(Stream &stream) {
    stream.Read(&g_nRndArenaLoadRevision, sizeof(g_nRndArenaLoadRevision));
    if (g_nRndArenaLoadRevision >= kArenaRejectedRevision) {
        g_failSink.Report("Can't load new Arena\n");
        return;
    }

    if (g_nRndArenaLoadRevision > 0) {
        Animatable::Load(stream);
    }
    if (g_nRndArenaLoadRevision >= kArenaBaseBlockRevision) {
        Collideable::Load(stream);
        Drawable::Load(stream);
        Transformable::Load(stream);
    }

    RemoveInstancesFromHitList();

    stream.Read(&mLoopDist.x, sizeof(mLoopDist.x));
    stream.Read(&mLoopDist.y, sizeof(mLoopDist.y));
    stream.Read(&mLoopDist.z, sizeof(mLoopDist.z));
    stream.Read(&mLoopFrames, sizeof(mLoopFrames));

    stream >> mSections;

    AddInstancesToHitList();
}

// 0x005b6338
void Arena::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);
    Drawable::Replace(pFrom, pTo);
    Transformable::Replace(pFrom, pTo);

    for (std::vector<Section>::iterator it = mSections.begin(); it != mSections.end(); ++it) {
        if (it->mView != pFrom) {
            continue;
        }
        if (pFrom != nullptr) {
            pFrom->RemoveRef(this);
        }
        if (it->mView != nullptr) {
            it->mView = pTo != nullptr ? dynamic_cast<View *>(pTo) : nullptr;
            if (it->mView != nullptr) {
                it->mView->AddRef(this);
            }
        }
    }

    // A matching entry is cleared rather than repointed at pTo, and no reference is dropped for
    // it. The section walk above already dropped the one reference this object stores.
    for (std::list<DrawEntry>::iterator it = mDrawOrder.begin(); it != mDrawOrder.end(); ++it) {
        if (it->mView == pFrom) {
            it->mView = nullptr;
        }
    }
}

// 0x005bbbb0
void Arena::Copy(const Object *pSource, unsigned nFlags) {
    // Yes, the binary reads through the cast result without testing it for null.
    const Arena *pArena = dynamic_cast<const Arena *>(pSource);
    Animatable::Copy(pSource, nFlags);
    Collideable::Copy(pSource, nFlags);
    Drawable::Copy(pSource, nFlags);
    Transformable::Copy(pSource, nFlags);
    RemoveInstancesFromHitList();
    mLoopDist = pArena->mLoopDist;
    mLoopFrames = pArena->mLoopFrames;
    mSections = pArena->mSections;
    AddInstancesToHitList();
}

// 0x005bbc98
void Arena::AddInstancesToHitList() {
    for (std::vector<Section>::iterator it = mSections.begin(); it != mSections.end(); ++it) {
        if (it->mView != nullptr) {
            it->mView->AddRef(this);
        }
    }
    mDrawOrder.resize(mSections.size(), DrawEntry());
}

// 0x005bbd30
void Arena::RemoveInstancesFromHitList() {
    for (std::vector<Section>::iterator it = mSections.begin(); it != mSections.end(); ++it) {
        if (it->mView != nullptr) {
            it->mView->RemoveRef(this);
        }
    }
    mDrawOrder.clear();
}

// 0x005b64e0
void Arena::UpdateSection(Section &section) {
    if (section.mView == nullptr) {
        return;
    }
    const float flFrame = mFilteredFrame;
    SetSectionLoop(section,
                   static_cast<int>(floorf((flFrame - section.mFrame) / mLoopFrames) + 1.0f));
    const float flLocalFrame = flFrame - (static_cast<float>(section.mLoop) * mLoopFrames);
    if (((section.mFrame - section.mDelta) <= flLocalFrame) && (flLocalFrame < section.mFrame)) {
        section.mView->SetFrame(flLocalFrame);
        section.mView->SetShowing(1);
    } else if (section.mView->GetShowing()) {
        section.mView->SetShowing(0);
        section.mView->StartAnim();
    }
}

// 0x005b7a90
void Arena::SetLoopDist(const Vector3 &loopDist) {
    if ((loopDist.x == mLoopDist.x) && (loopDist.y == mLoopDist.y) && (loopDist.z == mLoopDist.z)) {
        return;
    }
    for (Section &section : mSections) {
        SetSectionLoop(section, 0);
    }
    mLoopDist = loopDist;
    SetFrameSelf(mFilteredFrame);
}

// 0x005b7c48
void Arena::SetLoopFrames(float flLoopFrames) {
    if (flLoopFrames == 0.0f) {
        g_failSink.Report("Can't set frames = 0\n");
        return;
    }
    if (flLoopFrames == mLoopFrames) {
        return;
    }
    for (Section &section : mSections) {
        SetSectionLoop(section, 0);
    }
    mLoopFrames = flLoopFrames;
    SetFrameSelf(mFilteredFrame);
}

// 0x005b7de0
void Arena::SetSectionCount(unsigned int nCount) {
    for (unsigned int i = nCount; i < mSections.size(); ++i) {
        Section &section = mSections[i];
        SetSectionLoop(section, 0);
        if (section.mView != nullptr) {
            section.mView->RemoveRef(this);
        }
    }
    mSections.resize(nCount, Section());
    mDrawOrder.resize(nCount, DrawEntry());
}

// 0x005b8020
void Arena::SetSectionView(int nIndex, View *pView) {
    Section &section = mSections[nIndex];
    SetSectionLoop(section, 0);
    if (section.mView != nullptr) {
        section.mView->RemoveRef(this);
    }
    section.mView = pView;
    if (pView != nullptr) {
        pView->AddRef(this);
    }
    UpdateSection(section);
}

// 0x005b8170
void Arena::SetSectionRange(int nIndex, float flStartFrame, float flEndFrame) {
    Section &section = mSections[nIndex];
    SetSectionLoop(section, 0);
    section.mFrame = flEndFrame;
    section.mDelta = flEndFrame - flStartFrame;
    UpdateSection(section);
}

// 0x005bbda8
void Arena::SetSectionLoop(Section &section, int nLoop) {
    if ((section.mView == nullptr) || (section.mLoop == nLoop)) {
        return;
    }
    float *pflTranslation = section.mView->mLocalXfm[kXfmTranslationRow];
    Vector3 position{
        pflTranslation[0], pflTranslation[1], pflTranslation[2], pflTranslation[kVec3PaddingFloat]};
    Vector3 offset{0.0f, 0.0f, 0.0f, 1.0f};
    if (section.mLoop < nLoop) {
        Vec3Scale(&mLoopDist.x, static_cast<float>(nLoop - section.mLoop), &offset.x);
        AddVec3(&position.x, &offset.x, &position.x);
    } else if (nLoop < section.mLoop) {
        Vec3Scale(&mLoopDist.x, static_cast<float>(section.mLoop - nLoop), &offset.x);
        Vec3Sub(&position.x, &offset.x, &position.x);
    }
    section.mLoop = nLoop;
    if (section.mTeleport != 0) {
        pflTranslation[0] = position.x;
        pflTranslation[1] = position.y;
        pflTranslation[2] = position.z;
        section.mView->mDirty = 1;
    }
}

// 0x005bbeb0
void Arena::SetFrameSelf(float flFrame) {
    (void)flFrame; // Yes, the binary places the sections from mFilteredFrame instead.
    std::list<DrawEntry>::iterator entry = mDrawOrder.begin();
    for (Section &section : mSections) {
        UpdateSection(section);
        entry->mView = section.mView;
        entry->mSortKey = section.mFrame + (static_cast<float>(section.mLoop) * mLoopFrames);
        if (section.mSortStart != 0) {
            entry->mSortKey -= section.mDelta;
        }
        ++entry;
    }
    mDrawOrder.sort();
}

// 0x005bbf80
int Arena::UpdateWorldXfm(Transformable *pParent, int nForce) {
    const int nChanged = Transformable::UpdateWorldXfm(pParent, nForce);
    for (Section &section : mSections) {
        if ((section.mView != nullptr) && section.mView->GetShowing()) {
            section.mView->UpdateWorldXfm(this, nChanged);
        }
    }
    return nChanged;
}

// 0x005bc020
int Arena::DrawSelf() {
    for (DrawEntry &entry : mDrawOrder) {
        if (entry.mView != nullptr) {
            entry.mView->Draw();
        }
    }
    return 1;
}

// 0x005bc090
void Arena::Collide(const Ray &ray, HitSink &sink) {
    for (Section &section : mSections) {
        if ((section.mView != nullptr) && section.mView->GetShowing()) {
            section.mView->Collide(ray, sink);
        }
    }
}

// 0x005bc128
void Arena::SetSectionTeleport(int nIndex, int nTeleport) {
    Section &section = mSections[nIndex];
    if ((section.mTeleport != nTeleport) && (section.mView != nullptr)) {
        const int nTrips = (nTeleport != 0) ? section.mLoop : -section.mLoop;
        Vector3 offset{0.0f, 0.0f, 0.0f, 1.0f};
        Vec3Scale(&mLoopDist.x, static_cast<float>(nTrips), &offset.x);
        float *pflTranslation = section.mView->mLocalXfm[kXfmTranslationRow];
        // The sum is built in a temporary whose padding float is 1.0, and the whole quadword is
        // stored back over the translation row.
        Vector3 position{0.0f, 0.0f, 0.0f, 1.0f};
        AddVec3(pflTranslation, &offset.x, &position.x);
        pflTranslation[0] = position.x;
        pflTranslation[1] = position.y;
        pflTranslation[2] = position.z;
        pflTranslation[kVec3PaddingFloat] = position.w;
        section.mView->mDirty = 1;
    }
    section.mTeleport = nTeleport;
}

// 0x005bb870
void *Arena::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kArenaTag);
}

// 0x005bb890
void Arena::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kArenaTag);
}

// 0x005bbb28
Arena *NewArena(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Arena" and the object is 0x150 bytes.
    return new Arena(name);
}

// 0x005bb8f0
Arena *NewArenaThroughHook(const HxStr &name) {
    return g_pfnNewArena(name);
}

// 0x005bba98
// The null test in the body is the conversion of an Arena pointer to its virtual Rnd::Object base
// rather than a check the source asks for.
Object *CreateRegisteredArena(const HxStr &name) {
    return g_pfnNewArena(name);
}

// 0x005bb8b0
void Arena::Init() {
    g_pfnNewArena = NewArena;
    g_manager.RegisterClass(g_arenaClassName, CreateRegisteredArena);
}

} // namespace Rnd
