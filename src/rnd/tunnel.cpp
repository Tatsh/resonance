#include "rnd/tunnel.h"

#include <algorithm>
#include <list>
#include <math.h>
#include <vector>

#include "math/color.h"
#include "math/transform.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transanim.h"

namespace {

// The unset slice value, a hand-written sentinel.
constexpr int kNoSlice = 99999999;

// Save() writes kTunnelRevision. Load() rejects kTunnelRejectedRevision and later, and anything
// before kTunnelOldestRevision.
constexpr int kTunnelRevision = 37;
constexpr int kTunnelRejectedRevision = 38;
constexpr int kTunnelOldestRevision = 33;
// Before this revision a discarded word follows the path name.
constexpr int kPathWordDroppedRevision = 34;
// The first revision that stores mUnknown60.
constexpr int kUnknown60Revision = 35;
// The first revisions that store the counts of the cell grid and the slice grid.
constexpr int kCellCountRevision = 36;
constexpr int kSliceCountRevision = 37;

} // namespace

namespace Rnd {

namespace {

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    const char *pszName = pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString;
    stream.WriteBytes(pszName, pObject->mName.mLen + 1);
}

template <class T>
void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(g_manager.Find(name));
}

// 0x00478570
Stream &WriteFloatVector(Stream &stream, const std::vector<float> &values) {
    const int nCount = values.size();
    stream.Write(&nCount, sizeof(nCount));
    for (const float flValue : values) {
        stream.Write(&flValue, sizeof(flValue));
    }
    return stream;
}

// 0x00472dd8
Stream &ReadFloatVector(Stream &stream, std::vector<float> &values) {
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    values.resize(nCount, 0.0f);
    for (float &flValue : values) {
        stream.Read(&flValue, sizeof(flValue));
    }
    return stream;
}

// 0x00472d20
Stream &WriteEventList(Stream &stream, const std::list<TunnelEvent> &events) {
    const int nCount = events.size();
    stream.Write(&nCount, sizeof(nCount));
    for (const TunnelEvent &event : events) {
        event.Save(stream);
    }
    return stream;
}

// 0x004786b8
Stream &ReadEventList(Stream &stream, std::list<TunnelEvent> &events) {
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    events.resize(nCount);
    for (TunnelEvent &event : events) {
        event.Load(stream);
    }
    return stream;
}

// 0x00478620
Stream &WriteSeekerVector(Stream &stream, const std::vector<TunnelSeeker> &seekers) {
    const int nCount = seekers.size();
    stream.Write(&nCount, sizeof(nCount));
    for (const TunnelSeeker &seeker : seekers) {
        seeker.Save(stream);
    }
    return stream;
}

// 0x004730b0
Stream &ReadSeekerVector(Stream &stream, std::vector<TunnelSeeker> &seekers) {
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    seekers.resize(nCount, TunnelSeeker());
    for (TunnelSeeker &seeker : seekers) {
        seeker.Load(stream);
    }
    return stream;
}

// The per-chain record SaveSectionMaterials() writes: the material by name and the colour of the
// first vertex of the finest level.
void SaveChainMaterial(Stream &stream, const TunnelMeshChain &chain) {
    WriteObjectRef(stream, chain.front()->mMat);
    const Color &color = chain.front()->mVertsOwner->mVerts.front().mColor;
    stream.Write(&color.r, sizeof(color.r))
        .Write(&color.g, sizeof(color.g))
        .Write(&color.b, sizeof(color.b))
        .Write(&color.a, sizeof(color.a));
}

// Read nCount records of SaveChainMaterial() and apply each one to the chain of the same index.
void LoadChainMaterials(Stream &stream, std::vector<TunnelMeshChain> &chains, int nCount) {
    const int nChainCount = chains.size();
    Mat *pMat = nullptr;
    for (int i = 0; i < nCount; ++i) {
        ReadObjectRef(stream, pMat);
        Color color;
        stream.Read(&color.r, sizeof(color.r))
            .Read(&color.g, sizeof(color.g))
            .Read(&color.b, sizeof(color.b))
            .Read(&color.a, sizeof(color.a));
        if (i < nChainCount) {
            chains[i].front()->SetMaterialChain(pMat);
            chains[i].front()->SetVertexColor(color);
        }
    }
}

} // namespace

// 0x00476a10
void Tunnel::Collide(const Ray &ray, HitSink &sink) {
    for (TunnelMeshChain &chain : mUnknowna4) {
        chain.Collide(ray, sink);
    }
}

// 0x00467f48
void Tunnel::Update() {
    // Yes, the binary takes these references without releasing earlier ones. ReleaseRefs() is the
    // counterpart the callers run first.
    if (mPath != nullptr) {
        mPath->AddRef(this);
    }
    for (TunnelEvent &event : mEvents) {
        if (event.mObject != nullptr) {
            event.mObject->AddRef(this);
        }
    }
    BuildMesh();
    for (std::vector<TunnelSeeker>::iterator it = mSeekers.begin(); it != mSeekers.end(); ++it) {
        it->SetTunnel(this, it - mSeekers.begin());
    }
}

// 0x00468020
void Tunnel::ReleaseRefs() {
    if (mPath != nullptr) {
        mPath->RemoveRef(this);
    }
    for (TunnelEvent &event : mEvents) {
        if (event.mObject != nullptr) {
            event.mObject->RemoveRef(this);
        }
    }
    for (TunnelSeeker &seeker : mSeekers) {
        seeker.ReleaseRefs();
    }
    ClearMaterialSectionLists();
}

// 0x004680e0
void Tunnel::Replace(Object *pFrom, Object *pTo) {
    Drawable::Replace(pFrom, pTo);
    Animatable::Replace(pFrom, pTo);
    Collideable::Replace(pFrom, pTo);
    if (mPath == pFrom && mPath != nullptr) {
        pFrom->RemoveRef(this);
        mPath = dynamic_cast<TransAnim *>(pTo);
        if (mPath != nullptr) {
            mPath->AddRef(this);
        }
    }
    std::list<TunnelEvent>::iterator it = mEvents.begin();
    while (it != mEvents.end()) {
        it->Replace(pFrom, pTo, this);
        if (it->mObject == nullptr) {
            it = mEvents.erase(it);
        } else {
            ++it;
        }
    }
    for (TunnelSeeker &seeker : mSeekers) {
        seeker.Replace(pFrom, pTo, this);
    }
}

// 0x004682a8
void Tunnel::Save(Stream &stream) {
    const int nRevision = kTunnelRevision;
    stream.Write(&nRevision, sizeof(nRevision));
    Drawable::Save(stream);
    Animatable::Save(stream);
    stream.Write(&mUnknown38, sizeof(mUnknown38));
    stream.Write(&mRingCount, sizeof(mRingCount));
    stream.Write(&mSliceCount, sizeof(mSliceCount));
    stream.Write(&mUnknown44, sizeof(mUnknown44));
    stream.Write(&mUnknown48, sizeof(mUnknown48));
    stream.Write(&mUnknown4c, sizeof(mUnknown4c));
    stream.Write(&mUnknown50, sizeof(mUnknown50));
    stream.Write(&mUnknown54, sizeof(mUnknown54));
    WriteObjectRef(stream, mPath);
    stream.Write(&mLaneChangeFrames, sizeof(mLaneChangeFrames));
    WriteFloatVector(stream, mUnknown68);
    WriteEventList(stream, mEvents);
    WriteSeekerVector(stream, mSeekers);
    stream.Write(&mUnknownbc, sizeof(mUnknownbc));
    stream.Write(&mUnknown60, sizeof(mUnknown60));
    SaveSectionMaterials(stream);
}

// 0x00468538
void Tunnel::Load(Stream &stream) {
    stream.Read(&g_nTunnelLoadVersion, sizeof(g_nTunnelLoadVersion));
    if (g_nTunnelLoadVersion >= kTunnelRejectedRevision) {
        g_failSink.Report("Can't load new Tunnel\n");
        return;
    }
    if (g_nTunnelLoadVersion < kTunnelOldestRevision) {
        g_failSink.Report("Can't load old Tunnel\n");
        return;
    }
    Drawable::Load(stream);
    Animatable::Load(stream);
    ReleaseRefs();
    stream.Read(&mUnknown38, sizeof(mUnknown38));
    stream.Read(&mRingCount, sizeof(mRingCount));
    stream.Read(&mSliceCount, sizeof(mSliceCount));
    stream.Read(&mUnknown44, sizeof(mUnknown44));
    stream.Read(&mUnknown48, sizeof(mUnknown48));
    stream.Read(&mUnknown4c, sizeof(mUnknown4c));
    stream.Read(&mUnknown50, sizeof(mUnknown50));
    stream.Read(&mUnknown54, sizeof(mUnknown54));
    ReadObjectRef(stream, mPath);
    if (g_nTunnelLoadVersion < kPathWordDroppedRevision) {
        int nDiscarded;
        stream.Read(&nDiscarded, sizeof(nDiscarded));
    }
    stream.Read(&mLaneChangeFrames, sizeof(mLaneChangeFrames));
    ReadFloatVector(stream, mUnknown68);
    ReadEventList(stream, mEvents);
    ReadSeekerVector(stream, mSeekers);
    stream.Read(&mUnknownbc, sizeof(mUnknownbc));
    if (g_nTunnelLoadVersion >= kUnknown60Revision) {
        stream.Read(&mUnknown60, sizeof(mUnknown60));
    }
    Update();
    LoadSectionMaterials(stream);
}

// 0x00468a78
void Tunnel::SaveSectionMaterials(Stream &stream) {
    const int nCellCount = mUnknowna4.size();
    stream.Write(&nCellCount, sizeof(nCellCount));
    for (const TunnelMeshChain &chain : mUnknowna4) {
        SaveChainMaterial(stream, chain);
    }
    const int nSliceCount = mUnknownb0.size();
    stream.Write(&nSliceCount, sizeof(nSliceCount));
    for (const TunnelMeshChain &chain : mUnknownb0) {
        SaveChainMaterial(stream, chain);
    }
}

// 0x00468da0
void Tunnel::LoadSectionMaterials(Stream &stream) {
    int nCellCount = mUnknowna4.size();
    int nSliceCount = mUnknownb0.size();
    if (g_nTunnelLoadVersion >= kCellCountRevision) {
        stream.Read(&nCellCount, sizeof(nCellCount));
    }
    LoadChainMaterials(stream, mUnknowna4, nCellCount);
    if (g_nTunnelLoadVersion >= kSliceCountRevision) {
        stream.Read(&nSliceCount, sizeof(nSliceCount));
    }
    LoadChainMaterials(stream, mUnknownb0, nSliceCount);
}

// 0x00476788
void Tunnel::Copy(const Object *pSource, unsigned nFlags) {
    // Yes, the binary dereferences the cast result without testing it.
    const Tunnel *pTunnel = dynamic_cast<const Tunnel *>(pSource);
    Drawable::Copy(pSource, nFlags);
    Animatable::Copy(pSource, nFlags);
    ReleaseRefs();
    mUnknown38 = pTunnel->mUnknown38;
    mRingCount = pTunnel->mRingCount;
    mSliceCount = pTunnel->mSliceCount;
    mUnknown44 = pTunnel->mUnknown44;
    mUnknown48 = pTunnel->mUnknown48;
    mUnknown4c = pTunnel->mUnknown4c;
    mUnknown50 = pTunnel->mUnknown50;
    mUnknown54 = pTunnel->mUnknown54;
    mPath = pTunnel->mPath;
    mLaneChangeFrames = pTunnel->mLaneChangeFrames;
    mUnknown68 = pTunnel->mUnknown68;
    mEvents = pTunnel->mEvents;
    mSeekers = pTunnel->mSeekers;
    mUnknown5c = pTunnel->mUnknown5c;
    mUnknown60 = pTunnel->mUnknown60;
    Update();
}

// 0x004770d0
void Tunnel::SetPath(TransAnim *pPath) {
    if (mPath != nullptr) {
        mPath->RemoveRef(this);
    }
    mPath = pPath;
    if (pPath != nullptr) {
        pPath->AddRef(this);
    }
    if (mPath != nullptr) {
        (void)mPath->EndFrame(); // Yes, the binary discards the result.
    }
    std::fill(mUnknown88.begin(), mUnknown88.end(), kNoSlice);
}

// 0x004775b0
void Tunnel::GetPathXfm(Transform *pOut, float flFrame) {
    if (mPath != nullptr) {
        mPath->EvalFrame(flFrame, &pOut->mBasisX.x, 1);
        return;
    }
    pOut->mBasisX.x = 1.0f;
    pOut->mBasisX.y = 0.0f;
    pOut->mBasisX.z = 0.0f;
    pOut->mBasisY.x = 0.0f;
    pOut->mBasisY.y = 1.0f;
    pOut->mBasisY.z = 0.0f;
    pOut->mBasisZ.x = 0.0f;
    pOut->mBasisZ.y = 0.0f;
    pOut->mBasisZ.z = 1.0f;
    pOut->mTranslation.x = 0.0f;
    pOut->mTranslation.y = 0.0f;
    pOut->mTranslation.z = 0.0f;
    pOut->mTranslation.w = 1.0f;
}

// 0x004772c8
void Tunnel::SetLaneChangeFrames(float flFrames) {
    mLaneChangeFrames = flFrames;
}

// 0x00477298
TunnelSeeker *Tunnel::GetSeeker(unsigned nIndex) {
    return nIndex < mSeekers.size() ? &mSeekers[nIndex] : nullptr;
}

// 0x0046cfd8
void Tunnel::ResizeSeekers(unsigned nCount) {
    for (TunnelSeeker &seeker : mSeekers) {
        seeker.ReleaseRefs();
    }
    mSeekers.resize(nCount, TunnelSeeker());
    for (unsigned i = 0; i < mSeekers.size(); ++i) {
        mSeekers[i].SetTunnel(this, i);
    }
}

// 0x00477160
void Tunnel::Configure(float flUnknown38,
                       int nRingCount,
                       int nSliceCount,
                       int nUnknown44,
                       float flUnknown48,
                       float flUnknown4c,
                       float flUnknown50,
                       float flUnknown54) {
    mUnknown38 = flUnknown38;
    mRingCount = nRingCount;
    mSliceCount = nSliceCount;
    mUnknown44 = nUnknown44;
    mUnknown48 = flUnknown48;
    mUnknown4c = flUnknown4c;
    mUnknown50 = flUnknown50;
    mUnknown54 = flUnknown54;
    for (TunnelSeeker &seeker : mSeekers) {
        seeker.ReleaseRefs();
    }
    BuildMesh();
    for (unsigned i = 0; i < mSeekers.size(); ++i) {
        mSeekers[i].SetTunnel(this, i);
    }
}

// 0x00476540
int Tunnel::FrameToSlice(float flFrame) {
    return static_cast<int>(floorf(flFrame * mUnknown98));
}

// 0x006eab10
HxStr g_tunnelClassName("Tunnel");

// 0x004763b8
const HxStr &Tunnel::ClassName() const {
    return g_tunnelClassName;
}

// 0x004768b8
void Tunnel::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Drawable::DumpText(sink);
    Animatable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    // The author never finished this block. It writes no member of the class, and the Collideable
    // base is not dumped either.
    sink.Print("[Tunnel]\n");
    sink.Print("TODO\n");
}

// 0x0046d180
void Tunnel::ApplyMeshLodScreenSizes(const std::vector<float> &screenSizes) {
    mUnknown68 = screenSizes;
    for (TunnelMeshChain &chain : mUnknowna4) {
        chain.SetScreenSizes(mUnknown68);
    }
    for (TunnelMeshChain &chain : mUnknownb0) {
        chain.SetScreenSizes(mUnknown68);
    }
}

// 0x0046acf0
void Tunnel::ClearMaterialSectionLists() {
    mUnknownb0.clear();
    mUnknowna4.clear();
}

// 0x0046db80
void Tunnel::ProjectSectionToCameraSpace(
    int nRing, Transform *pOut, float flAnimFrame, float flRingBlend, float flTangentScale) {
    if (mPath == nullptr) {
        // Yes, the padding words of the three basis rows are not written, while the translation
        // row is stored whole as (0, 0, 0, 1).
        pOut->mBasisX.x = 1.0f;
        pOut->mBasisX.y = 0.0f;
        pOut->mBasisX.z = 0.0f;
        pOut->mBasisY.x = 0.0f;
        pOut->mBasisY.y = 1.0f;
        pOut->mBasisY.z = 0.0f;
        pOut->mBasisZ.x = 0.0f;
        pOut->mBasisZ.y = 0.0f;
        pOut->mBasisZ.z = 1.0f;
        pOut->mTranslation.x = 0.0f;
        pOut->mTranslation.y = 0.0f;
        pOut->mTranslation.z = 0.0f;
        pOut->mTranslation.w = 1.0f;
        return;
    }
    const Transform &ring = mUnknownc0[nRing];
    pOut->mBasisX = ring.mBasisX;
    pOut->mBasisY = ring.mBasisY;
    pOut->mBasisZ = ring.mBasisZ;

    Vector3 current;
    current.w = 1.0f;
    Vec3Scale(&ring.mTranslation.x, flTangentScale, &current.x);
    Vector3 next;
    next.w = 1.0f;
    Vec3Scale(
        &mUnknownc0[WrapIndex(nRing + 1, mRingCount)].mTranslation.x, flTangentScale, &next.x);
    // A VU0 multiply and accumulate in the image, as in LerpRingSectionTangent().
    const float flComplement = 1.0f - flRingBlend;
    pOut->mTranslation.x = next.x * flRingBlend + current.x * flComplement;
    pOut->mTranslation.y = next.y * flRingBlend + current.y * flComplement;
    pOut->mTranslation.z = next.z * flRingBlend + current.z * flComplement;
    pOut->mTranslation.w = next.w;

    Transform anim;
    anim.mBasisX.w = 1.0f;
    anim.mBasisY.w = 1.0f;
    anim.mBasisZ.w = 1.0f;
    anim.mTranslation.w = 1.0f;
    mPath->EvalFrame(flAnimFrame, &anim.mBasisX.x, 1);
    // Yes, the output is also the first input.
    XfmConcat(&pOut->mBasisX.x, &anim.mBasisX.x, &pOut->mBasisX.x);
}

// 0x004773e8
Mesh *Tunnel::GetRingSection(int nSlice) {
    return mUnknownb0[WrapIndex(nSlice, mSliceCount)].front();
}

// 0x00477388
Mesh *Tunnel::GetRingSection(int nRing, int nSlice) {
    return mUnknowna4[WrapIndex(nSlice, mSliceCount) * mRingCount + WrapIndex(nRing, mRingCount)]
        .front();
}

// 0x00477538. A VU0 multiply and accumulate in the image, vmulax then vmaddx over xyz.
void Tunnel::LerpRingSectionTangent(int nRing, Vector3 *pOut, float flWeight) {
    const Vector3 &next = mUnknownc0[WrapIndex(nRing + 1, mRingCount)].mTranslation;
    const Vector3 &current = mUnknownc0[nRing].mTranslation;
    const float flComplement = 1.0f - flWeight;
    pOut->x = next.x * flWeight + current.x * flComplement;
    pOut->y = next.y * flWeight + current.y * flComplement;
    pOut->z = next.z * flWeight + current.z * flComplement;
    pOut->w = next.w;
}

// 0x00476f48
void Tunnel::ScrollRings() {
    for (int nSlice = mUnknownbc; nSlice < mUnknownbc + mSliceCount; ++nSlice) {
        if (mUnknown88[WrapIndex(nSlice, mSliceCount)] != nSlice) {
            AdvanceRing(nSlice);
            return;
        }
    }
}

// 0x00476fe0
void Tunnel::AdvanceRing(int nSlice) {
    const int nIndex = WrapIndex(nSlice, mSliceCount);
    if (nSlice != mUnknown7c) {
        mUnknown88[nIndex] = kNoSlice;
        mUnknown7c = nSlice;
        mUnknown84 = mUnknowna0;
        mUnknown80 = nSlice * mUnknown9c;
    }
    SetRingSectionFrames();
    if (mUnknown84 == 0) {
        mUnknown88[nIndex] = nSlice;
        mUnknown7c = kNoSlice;
    } else {
        --mUnknown84;
        mUnknown80 += mUnknown9c / mUnknowna0;
    }
}

// 0x0046d400
void Tunnel::AddEvent(Drawable *pObject, float flFrame, int nId, int nUser) {
    std::list<TunnelEvent>::iterator it = mEvents.begin();
    while (it != mEvents.end() && !(flFrame <= it->mFrame)) {
        ++it;
    }
    it = mEvents.insert(it, TunnelEvent(pObject, flFrame, nId, nUser));
    if (it->mObject != nullptr) {
        it->mObject->AddRef(this);
    }
}

// 0x0046d540
int Tunnel::MoveEvent(int nId, float flFrame) {
    for (std::list<TunnelEvent>::iterator it = mEvents.begin(); it != mEvents.end(); ++it) {
        if (it->mId == nId) {
            Drawable *pObject = it->mObject;
            mEvents.erase(it);
            // Yes, the reference taken for the original entry is not dropped.
            AddEvent(pObject, flFrame, nId, 0);
            return 1;
        }
    }
    return 0;
}

// 0x0046d5d8
int Tunnel::RemoveEvent(int nId) {
    for (std::list<TunnelEvent>::iterator it = mEvents.begin(); it != mEvents.end(); ++it) {
        if (it->mId == nId) {
            if (it->mObject != nullptr) {
                it->mObject->RemoveRef(this);
            }
            mEvents.erase(it);
            return 1;
        }
    }
    return 0;
}

// 0x0046d680
int Tunnel::RemoveEventsInRange(float flFrom, float flTo) {
    int nRemoved = 0;
    std::list<TunnelEvent>::iterator it = mEvents.begin();
    while (it != mEvents.end()) {
        if (it->mFrame < flTo && flFrom <= it->mFrame) {
            if (it->mObject != nullptr) {
                it->mObject->RemoveRef(this);
            }
            ++nRemoved;
            it = mEvents.erase(it);
        } else {
            ++it;
        }
    }
    return nRemoved;
}

// 0x00477310
void Tunnel::ForEachEvent(void (*pfnVisit)(Drawable *pObject, float flFrame, int nId, void *pUser),
                          void *pUser) {
    for (const TunnelEvent &event : mEvents) {
        pfnVisit(event.mObject, event.mFrame, event.mId, pUser);
    }
}

// 0x00476288
Tunnel *NewTunnel(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Tunnel" and the object is 0x104 bytes.
    return new Tunnel(name);
}

// 0x00476468
Object *CreateRegisteredTunnel(const HxStr &name) {
    return new Tunnel(name);
}

// 0x00894d64
int g_nTunnelLoadVersion;

} // namespace Rnd
