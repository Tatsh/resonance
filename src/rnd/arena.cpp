#include "rnd/arena.h"

#include <list>
#include <math.h>
#include <vector>

#include "math/vector3.h"
#include "os/dbg.h"
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

constexpr float kDefaultLoopFrames = 1000.0f;

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

void PrintObjectRef(Dbg &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print(kNoObject);
        return;
    }
    sink.Format(kQuotedTextFormat, NameText(pObject));
}

void PrintBool(Dbg &sink, int nValue) {
    sink.Print(nValue != 0 ? kTrueText : kFalseText);
}

// The padding word of the vector is not written.
void PrintVector(Dbg &sink, const Vector3 &vec) {
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
        stream.Write(&chTerminator, sizeof(chTerminator));
        return;
    }
    stream.Write(NameText(pObject), pObject->mName.mLen + 1);
}

// A byte is written for each of the two section flags even though both are stored as words.
void WriteBool(Stream &stream, int nValue) {
    const char chFlag = static_cast<char>(nValue);
    stream.Write(&chFlag, sizeof(chFlag));
}

int ReadBool(Stream &stream) {
    char chFlag = 0;
    stream.Read(&chFlag, sizeof(chFlag));
    return chFlag != 0 ? 1 : 0;
}

} // namespace

// NTSC-U/C: 0x00777100, PAL: 0x007bafe8
HxStr g_arenaClassName("Arena");

// NTSC-U/C: 0x008e4a4c, PAL: 0x00929a74
int g_nRndArenaLoadRevision;

// NTSC-U/C: 0x00777108, PAL: 0x007baff0
Arena *(*g_pfnNewArena)(const HxStr &name);

// NTSC-U/C: 0x005b82b0, PAL: 0x005fa918
static Dbg &operator<<(Dbg &sink, const Arena::Section &section) {
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

// NTSC-U/C: 0x005ba640, PAL: 0x005fccf8
static Dbg &operator<<(Dbg &sink, const std::vector<Arena::Section> &sections) {
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

// NTSC-U/C: 0x005ba750, PAL: 0x005fce08
static Stream &operator<<(Stream &stream, const std::vector<Arena::Section> &sections) {
    const int nCount = sections.size();
    stream.WriteLE(&nCount, sizeof(nCount));

    for (std::vector<Arena::Section>::const_iterator it = sections.begin(); it != sections.end();
         ++it) {
        WriteObjectRef(stream, it->mView);
        stream.WriteLE(&it->mFrame, sizeof(it->mFrame));
        stream.WriteLE(&it->mLoop, sizeof(it->mLoop));
        stream.WriteLE(&it->mDelta, sizeof(it->mDelta));
        WriteBool(stream, it->mTeleport);
        WriteBool(stream, it->mSortStart);
    }
    return stream;
}

// NTSC-U/C: 0x005b8450, PAL: 0x005faab8
static Stream &operator>>(Stream &stream, Arena::Section &section) {
    HxStr viewName(nullptr);
    stream.ReadString(viewName);
    section.mView = dynamic_cast<View *>(TheManager.Find(viewName));

    stream.ReadLE(&section.mFrame, sizeof(section.mFrame));
    stream.ReadLE(&section.mLoop, sizeof(section.mLoop));

    if (g_nRndArenaLoadRevision >= kArenaSectionDeltaRevision) {
        stream.ReadLE(&section.mDelta, sizeof(section.mDelta));
    }
    if (g_nRndArenaLoadRevision >= kArenaTeleportRevision) {
        section.mTeleport = ReadBool(stream);
    }
    if (g_nRndArenaLoadRevision >= kArenaSortStartRevision) {
        section.mSortStart = ReadBool(stream);
    }
    return stream;
}

// NTSC-U/C: 0x005ba908, PAL: 0x005fcfc0
static Stream &operator>>(Stream &stream, std::vector<Arena::Section> &sections) {
    int nCount = 0;
    stream.ReadLE(&nCount, sizeof(nCount));

    sections.resize(nCount, Arena::Section());

    for (std::vector<Arena::Section>::iterator it = sections.begin(); it != sections.end(); ++it) {
        stream >> *it;
    }
    return stream;
}

Arena::~Arena() {
    RemoveInstancesFromHitList();
    ReleaseAllRefs();
}

Arena::Arena(const HxStr &name)
    : Object(name), mLoopDist{0.0f, 0.0f, 0.0f, 1.0f}, mLoopFrames(kDefaultLoopFrames) {
    AddInstancesToHitList();
}

const HxStr &Arena::ClassName() const {
    return g_arenaClassName;
}

Vector3 &Arena::LoopDist() {
    return mLoopDist;
}

void Arena::DumpText(Dbg &sink) {
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

void Arena::Save(Stream &stream) {
    const int nRevision = kArenaRevision;
    stream.WriteLE(&nRevision, sizeof(nRevision));

    Animatable::Save(stream);
    Collideable::Save(stream);
    Drawable::Save(stream);
    Transformable::Save(stream);

    stream.WriteLE(&mLoopDist.x, sizeof(mLoopDist.x));
    stream.WriteLE(&mLoopDist.y, sizeof(mLoopDist.y));
    stream.WriteLE(&mLoopDist.z, sizeof(mLoopDist.z));
    stream.WriteLE(&mLoopFrames, sizeof(mLoopFrames));

    stream << mSections;
}

void Arena::Load(Stream &stream) {
    stream.ReadLE(&g_nRndArenaLoadRevision, sizeof(g_nRndArenaLoadRevision));
    if (g_nRndArenaLoadRevision >= kArenaRejectedRevision) {
        Rnd::TheDbg.Notify("Can't load new Arena\n");
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

    stream.ReadLE(&mLoopDist.x, sizeof(mLoopDist.x));
    stream.ReadLE(&mLoopDist.y, sizeof(mLoopDist.y));
    stream.ReadLE(&mLoopDist.z, sizeof(mLoopDist.z));
    stream.ReadLE(&mLoopFrames, sizeof(mLoopFrames));

    stream >> mSections;

    AddInstancesToHitList();
}

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

void Arena::AddInstancesToHitList() {
    for (std::vector<Section>::iterator it = mSections.begin(); it != mSections.end(); ++it) {
        if (it->mView != nullptr) {
            it->mView->AddRef(this);
        }
    }
    mDrawOrder.resize(mSections.size(), DrawEntry());
}

void Arena::RemoveInstancesFromHitList() {
    for (std::vector<Section>::iterator it = mSections.begin(); it != mSections.end(); ++it) {
        if (it->mView != nullptr) {
            it->mView->RemoveRef(this);
        }
    }
    mDrawOrder.clear();
}

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

void Arena::SetLoopFrames(float flLoopFrames) {
    if (flLoopFrames == 0.0f) {
        Rnd::TheDbg.Notify("Can't set frames = 0\n");
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

void Arena::SetSectionRange(int nIndex, float flStartFrame, float flEndFrame) {
    Section &section = mSections[nIndex];
    SetSectionLoop(section, 0);
    section.mFrame = flEndFrame;
    section.mDelta = flEndFrame - flStartFrame;
    UpdateSection(section);
}

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
        Rnd::Add(&position.x, &offset.x, &position.x);
    } else if (nLoop < section.mLoop) {
        Vec3Scale(&mLoopDist.x, static_cast<float>(section.mLoop - nLoop), &offset.x);
        Rnd::Subtract(&position.x, &offset.x, &position.x);
    }
    section.mLoop = nLoop;
    if (section.mTeleport != 0) {
        pflTranslation[0] = position.x;
        pflTranslation[1] = position.y;
        pflTranslation[2] = position.z;
        section.mView->mDirty = 1;
    }
}

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

int Arena::UpdateWorldXfm(Transformable *pParent, int nForce) {
    const int nChanged = Transformable::UpdateWorldXfm(pParent, nForce);
    for (Section &section : mSections) {
        if ((section.mView != nullptr) && section.mView->GetShowing()) {
            section.mView->UpdateWorldXfm(this, nChanged);
        }
    }
    return nChanged;
}

int Arena::DrawShowing() {
    for (DrawEntry &entry : mDrawOrder) {
        if (entry.mView != nullptr) {
            entry.mView->Draw();
        }
    }
    return 1;
}

void Arena::FindCollisions(const Segment &ray, std::list<Collision> &collisions) {
    for (Section &section : mSections) {
        if ((section.mView != nullptr) && section.mView->GetShowing()) {
            section.mView->FindCollisions(ray, collisions);
        }
    }
}

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
        Rnd::Add(pflTranslation, &offset.x, &position.x);
        pflTranslation[0] = position.x;
        pflTranslation[1] = position.y;
        pflTranslation[2] = position.z;
        pflTranslation[kVec3PaddingFloat] = position.w;
        section.mView->mDirty = 1;
    }
    section.mTeleport = nTeleport;
}

void *Arena::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kArenaTag);
}

void Arena::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kArenaTag);
}

Arena *NewArena(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Arena" and the object is 0x150 bytes.
    return new Arena(name);
}

Arena *NewArenaThroughHook(const HxStr &name) {
    try {
        return g_pfnNewArena(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

Object *CreateRegisteredArena(const HxStr &name) {
    // The null test in the body is the conversion of an Arena pointer to its virtual Rnd::Object
    // base rather than a check the source requests.
    try {
        return g_pfnNewArena(name);
    } catch (...) {
        return nullptr;
    }
}

void Arena::Init() {
    g_pfnNewArena = NewArena;
    TheManager.RegisterClass(g_arenaClassName, CreateRegisteredArena);
}

} // namespace Rnd
