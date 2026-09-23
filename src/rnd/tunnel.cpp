#include "rnd/tunnel.h"

#include <algorithm>
#include <list>
#include <math.h>
#include <vector>

#include "app/longop.h"
#include "math/color.h"
#include "math/transform.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"
#include "rnd/object.h"
#include "rnd/stream.h"
#include "rnd/transanim.h"
#include "rnd/tunnelsortentry.h"

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
// The first revision that stores mCulledFarSlices.
constexpr int kUnknown60Revision = 35;
// The first revisions that store the counts of the cell grid and the slice grid.
constexpr int kCellCountRevision = 36;
constexpr int kSliceCountRevision = 37;

// The frames one slice spans.
constexpr float kSliceFrames = 1920.0f;
// The value of pi the image uses, one unit in the last place below the nearest float.
constexpr float kPi = 3.1415925f;
// The capacity the constructor reserves for the seeker vector.
constexpr int kInitialSeekerCapacity = 4;

// A lane block of a slice mesh holds, per ring, three panels of two rows of mSliceSteps + 1
// vertices, one for each segment of the lane profile, followed by two end caps of two rows of two.
constexpr int kLaneSegmentCount = 3;
constexpr int kPanelRows = 2;
constexpr int kCapRows = 2;
constexpr int kCapColumns = 2;
constexpr int kCapVerts = kCapRows * kCapColumns;
// The panels of a block, in the order the profile segments run.
enum LanePanel {
    kLanePanelLeftWall = 0,  // mPoints[0] to mPoints[1]
    kLanePanelFloor = 1,     // mPoints[1] to mPoints[2]
    kLanePanelRightWall = 2, // mPoints[2] to mPoints[3]
};

// BuildLaneMeshes() gives each lane one flat grid of four rows instead of three panels, and its
// end caps these texture ranges.
constexpr int kFlatLaneRows = 4;
constexpr float kFlatLaneCapFirstU = 0.666f;
constexpr float kFlatLaneCapStepU = 0.334f;
constexpr float kFlatLaneCap2FirstU = 0.333f;
constexpr float kFlatLaneCap2StepU = 0.333f;

const char *const kTunnelTag = "Rnd::Tunnel";

const char kSliceNameFormat[] = "%s_lat%03d";
const char kCellNameFormat[] = "%s_pan%03d";

} // namespace

namespace Rnd {

namespace {

const char *NameText(const Object *pObject) {
    return pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString;
}

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, 1);
        return;
    }
    stream.WriteBytes(NameText(pObject), pObject->mName.mLen + 1);
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

// out.xyz = a.xyz * flA + b.xyz * flB, with out.w taken from a. A VU0 multiply and accumulate in
// the image. out may alias either input.
inline void BlendVector(const Vector3 &a, float flA, const Vector3 &b, float flB, Vector3 &out) {
    const float flX = a.x * flA + b.x * flB;
    const float flY = a.y * flA + b.y * flB;
    const float flZ = a.z * flA + b.z * flB;
    const float flW = a.w;
    out.x = flX;
    out.y = flY;
    out.z = flZ;
    out.w = flW;
}

// A point carried through a transform, the basis rows weighted by its components plus the
// translation, with out.w taken from the input. A VU0 multiply and accumulate in the image.
inline void XfmPoint(const Transform &xfm, const Vector3 &in, Vector3 &out) {
    const float flX =
        xfm.mBasisX.x * in.x + xfm.mBasisY.x * in.y + xfm.mBasisZ.x * in.z + xfm.mTranslation.x;
    const float flY =
        xfm.mBasisX.y * in.x + xfm.mBasisY.y * in.y + xfm.mBasisZ.y * in.z + xfm.mTranslation.y;
    const float flZ =
        xfm.mBasisX.z * in.x + xfm.mBasisY.z * in.y + xfm.mBasisZ.z * in.z + xfm.mTranslation.z;
    const float flW = in.w;
    out.x = flX;
    out.y = flY;
    out.z = flZ;
    out.w = flW;
}

// A direction carried through the basis rows alone, with out.w taken from the input.
inline void XfmVector(const Transform &xfm, const Vector3 &in, Vector3 &out) {
    const float flX = xfm.mBasisX.x * in.x + xfm.mBasisY.x * in.y + xfm.mBasisZ.x * in.z;
    const float flY = xfm.mBasisX.y * in.x + xfm.mBasisY.y * in.y + xfm.mBasisZ.y * in.z;
    const float flZ = xfm.mBasisX.z * in.x + xfm.mBasisY.z * in.y + xfm.mBasisZ.z * in.z;
    const float flW = in.w;
    out.x = flX;
    out.y = flY;
    out.z = flZ;
    out.w = flW;
}

// Every transform and vector the geometry routines build on the stack starts with its padding
// words at 1.0 and nothing else written.
inline void SetPaddingWords(Transform &xfm) {
    xfm.mBasisX.w = 1.0f;
    xfm.mBasisY.w = 1.0f;
    xfm.mBasisZ.w = 1.0f;
    xfm.mTranslation.w = 1.0f;
}

// Give rows of vertices their texture coordinates. The horizontal coordinate steps by flRowStep
// from row to row, and the vertical one runs from 0 to 1 along each row.
inline void SetGridTexCoords(
    std::vector<MeshVert> &verts, int nFirst, int nRows, int nColumns, float flRowStep) {
    const float flStep = 1.0f / (nColumns - 1);
    float flU = 0.0f;
    for (int nRow = 0; nRow < nRows; ++nRow) {
        float flV = 0.0f;
        for (int nColumn = 0; nColumn < nColumns; ++nColumn) {
            MeshVert &vert = verts[nFirst + nRow * nColumns + nColumn];
            vert.mTex1.y = flV;
            vert.mTex1.x = flU;
            flV += flStep;
        }
        flU += flRowStep;
    }
}

// Give the four vertices of a lane end cap their texture coordinates. The horizontal coordinate
// runs from flFirstU by flStepU across a row, and the vertical one is the row.
inline void
SetCapTexCoords(std::vector<MeshVert> &verts, int nFirst, float flFirstU, float flStepU) {
    float flV = 0.0f;
    for (int nRow = 0; nRow < kCapRows; ++nRow) {
        float flU = flFirstU;
        for (int nColumn = 0; nColumn < kCapColumns; ++nColumn) {
            MeshVert &vert = verts[nFirst + nRow * kCapColumns + nColumn];
            vert.mTex1.x = flU;
            vert.mTex1.y = flV;
            flU += flStepU;
        }
        flV += 1.0f;
    }
}

inline void FillColor(std::vector<MeshVert> &verts, int nFirst, int nCount, const Color &color) {
    for (int i = nFirst; i < nFirst + nCount; ++i) {
        verts[i].mColor = color;
    }
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
    stream.Write(&mLodCount, sizeof(mLodCount));
    stream.Write(&mUnknown48, sizeof(mUnknown48));
    stream.Write(&mUnknown4c, sizeof(mUnknown4c));
    stream.Write(&mUnknown50, sizeof(mUnknown50));
    stream.Write(&mUnknown54, sizeof(mUnknown54));
    WriteObjectRef(stream, mPath);
    stream.Write(&mLaneChangeFrames, sizeof(mLaneChangeFrames));
    WriteFloatVector(stream, mLodScreenSizes);
    WriteEventList(stream, mEvents);
    WriteSeekerVector(stream, mSeekers);
    stream.Write(&mUnknownbc, sizeof(mUnknownbc));
    stream.Write(&mCulledFarSlices, sizeof(mCulledFarSlices));
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
    stream.Read(&mLodCount, sizeof(mLodCount));
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
    ReadFloatVector(stream, mLodScreenSizes);
    ReadEventList(stream, mEvents);
    ReadSeekerVector(stream, mSeekers);
    stream.Read(&mUnknownbc, sizeof(mUnknownbc));
    if (g_nTunnelLoadVersion >= kUnknown60Revision) {
        stream.Read(&mCulledFarSlices, sizeof(mCulledFarSlices));
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
    mLodCount = pTunnel->mLodCount;
    mUnknown48 = pTunnel->mUnknown48;
    mUnknown4c = pTunnel->mUnknown4c;
    mUnknown50 = pTunnel->mUnknown50;
    mUnknown54 = pTunnel->mUnknown54;
    mPath = pTunnel->mPath;
    mLaneChangeFrames = pTunnel->mLaneChangeFrames;
    mLodScreenSizes = pTunnel->mLodScreenSizes;
    mEvents = pTunnel->mEvents;
    mSeekers = pTunnel->mSeekers;
    mUnknown5c = pTunnel->mUnknown5c;
    mCulledFarSlices = pTunnel->mCulledFarSlices;
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
                       int nLodCount,
                       float flUnknown48,
                       float flUnknown4c,
                       float flUnknown50,
                       float flUnknown54) {
    mUnknown38 = flUnknown38;
    mRingCount = nRingCount;
    mSliceCount = nSliceCount;
    mLodCount = nLodCount;
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
    return static_cast<int>(floorf(flFrame * mSlicesPerFrame));
}

// 0x00466620
Tunnel::Tunnel(const HxStr &name)
    : Object(name), mUnknown38(1.0f), mRingCount(3), mSliceCount(0), mLodCount(2), mUnknown48(0.1f),
      mUnknown4c(0.1f), mUnknown50(0.25f), mUnknown54(0.01f), mPath(nullptr), mUnknown5c(0),
      mCulledFarSlices(0), mLaneChangeFrames(480.0f), mDrawLattice(1), mDrawPanels(1),
      mUnknown7c(kNoSlice), mUnknown80(0.0f), mUnknown84(0), mSlicesPerFrame(0.0f),
      mSliceFrames(0.0f), mUnknownbc(0) {
    mSeekers.reserve(kInitialSeekerCapacity);
    mLodScreenSizes.resize(mLodCount, 0.0f);
    std::fill(mLodScreenSizes.begin(), mLodScreenSizes.end(), 0);
    Update();
}

// 0x004676b0
Tunnel::~Tunnel() {
    ReleaseRefs();
    ReleaseAllRefs();
}

// 0x00476218
void *Tunnel::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kTunnelTag);
}

// 0x00476238
void Tunnel::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kTunnelTag);
}

// 0x0046da40
void Tunnel::GetRingXfm(int nRing, Transform *pOut, float flFrame, float flBlend) {
    if (mPath == nullptr) {
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
    LerpRingSectionTangent(nRing, &pOut->mTranslation, flBlend);
    Transform anim;
    SetPaddingWords(anim);
    mPath->EvalFrame(flFrame, &anim.mBasisX.x, 1);
    // Yes, the output is also the first input.
    XfmConcat(&pOut->mBasisX.x, &anim.mBasisX.x, &pOut->mBasisX.x);
}

// 0x00468850
int Tunnel::DrawSelf() {
    const int nEnd = mUnknownbc + mSliceCount - mCulledFarSlices;
    if (mDrawLattice != 0) {
        for (int nSlice = nEnd - 1; nSlice >= mUnknownbc; --nSlice) {
            const int nIndex = nSlice % mSliceCount;
            if (mUnknown88[nIndex] != kNoSlice) {
                mUnknownb0[nIndex].Draw(nSlice * mSliceFrames - mFilteredFrame);
            }
        }
    }
    if (mDrawPanels != 0) {
        for (int nSlice = nEnd - 1; nSlice >= mUnknownbc; --nSlice) {
            const float flDistance = nSlice * mSliceFrames - mFilteredFrame;
            const int nIndex = nSlice % mSliceCount;
            if (mUnknown88[nIndex] == nSlice) {
                for (int nRing = 0; nRing < mRingCount; ++nRing) {
                    mUnknowna4[nIndex * mRingCount + nRing].Draw(flDistance);
                }
                for (TunnelSeeker &seeker : mSeekers) {
                    seeker.DrawSection(nSlice, flDistance);
                }
            }
        }
    }
    for (TunnelSeeker &seeker : mSeekers) {
        seeker.DrawMesh();
    }
    return 1;
}

// 0x00469180
void Tunnel::SetFrameSelf(float flFrame) {
    float flEarliestOffset = 0.0f;
    for (const TunnelSeeker &seeker : mSeekers) {
        flEarliestOffset = std::min(flEarliestOffset, seeker.mTransFrameOffset);
    }
    mUnknownbc = static_cast<int>(floorf((flFrame + flEarliestOffset) * mSlicesPerFrame));
    if (mUnknownbc < 0) {
        mUnknownbc = 0;
    }
    ScrollRings();
    if (mPath == nullptr) {
        return;
    }

    for (TunnelSeeker &seeker : mSeekers) {
        Transform trans;
        SetPaddingWords(trans);
        Transform look;
        SetPaddingWords(look);
        Transform meshXfm;
        SetPaddingWords(meshXfm);
        mPath->EvalFrame(flFrame + seeker.mTransFrameOffset, &trans.mBasisX.x, 1);
        mPath->EvalFrame(flFrame + seeker.mLookFrameOffset, &look.mBasisX.x, 1);

        // Turn the look frame about its vertical axis to the seeker's lane and aim the
        // transformable from its own path point at the lane, mUnknown38 out from the axis.
        Transform aim;
        SetPaddingWords(aim);
        const float flAngle = -seeker.UpdateLane() * 2.0f * kPi / mRingCount;
        const float flCos = cosf(flAngle);
        const float flSin = sinf(flAngle);
        aim.mBasisZ.x = flSin;
        aim.mBasisZ.z = flCos;
        aim.mBasisX.x = flCos;
        aim.mBasisX.z = -flSin;
        aim.mBasisX.y = 0.0f;
        aim.mBasisY.x = 0.0f;
        aim.mBasisY.y = 1.0f;
        aim.mBasisY.z = 0.0f;
        aim.mBasisZ.y = 0.0f;
        aim.mTranslation.x = 0.0f;
        aim.mTranslation.y = 0.0f;
        aim.mTranslation.z = 0.0f;
        aim.mTranslation.w = 1.0f;
        sceVu0Sub005e7ab0(&aim.mBasisX.x, &look.mBasisX.x, &aim.mBasisX.x);

        Vector3 lanePoint;
        lanePoint.x = 0.0f;
        lanePoint.y = 0.0f;
        lanePoint.z = -mUnknown38;
        lanePoint.w = 1.0f;
        XfmPoint(aim, lanePoint, aim.mTranslation);
        Vector3 direction;
        direction.w = 1.0f;
        Vec3Sub(&aim.mTranslation.x, &trans.mTranslation.x, &direction.x);
        Mat33BuildOrthonormal(&direction.x, &aim.mBasisZ.x, &aim.mBasisX.x);
        aim.mTranslation = trans.mTranslation;
        seeker.SetTransXfm(aim);

        mPath->EvalFrame(flFrame + seeker.mMeshFrameOffset, &meshXfm.mBasisX.x, 1);
        seeker.SetMeshXfm(meshXfm);
    }
}

// 0x004699c0
void Tunnel::BuildMesh() {
    std::vector<Mat *> cellMats(mUnknowna4.size(), nullptr);
    std::vector<Color> cellColors(mUnknowna4.size());
    for (unsigned i = 0; i < mUnknowna4.size(); ++i) {
        cellMats[i] = mUnknowna4[i].front()->mMat;
        cellColors[i] = mUnknowna4[i].front()->mVertsOwner->mVerts.front().mColor;
    }
    std::vector<Mat *> sliceMats(mUnknownb0.size(), nullptr);
    std::vector<Color> sliceColors(mUnknownb0.size());
    for (unsigned i = 0; i < mUnknownb0.size(); ++i) {
        sliceMats[i] = mUnknownb0[i].front()->mMat;
        sliceColors[i] = mUnknownb0[i].front()->mVertsOwner->mVerts.front().mColor;
    }
    ClearMaterialSectionLists();

    mSliceFrames = kSliceFrames;
    mSlicesPerFrame = 1.0f / kSliceFrames;
    mUnknown88.resize(mSliceCount, 0);
    std::fill(mUnknown88.begin(), mUnknown88.end(), kNoSlice);

    // Yes, the binary fills the new elements from blanks with only their padding words written.
    // The loops below overwrite everything else.
    Transform blankXfm;
    SetPaddingWords(blankXfm);
    mUnknownc0.resize(mRingCount, blankXfm);
    LaneProfile blankProfile;
    for (Vector3 &point : blankProfile.mPoints) {
        point.w = 1.0f;
    }
    for (Vector3 &normal : blankProfile.mNormals) {
        normal.w = 1.0f;
    }
    mLaneProfiles.resize(mRingCount, blankProfile);

    // Each ring faces its own angle, with its translation half a ring back around the axis.
    const float flHalfRing = kPi / mRingCount;
    float flAngle = 0.0f;
    for (int nRing = 0; nRing < mRingCount; ++nRing) {
        Transform &xfm = mUnknownc0[nRing];
        const float flCos = cosf(flAngle);
        const float flSin = sinf(flAngle);
        xfm.mBasisX.x = flCos;
        xfm.mBasisX.z = flSin;
        xfm.mBasisX.y = 0.0f;
        xfm.mBasisY.x = 0.0f;
        xfm.mBasisY.z = 0.0f;
        xfm.mBasisY.y = 1.0f;
        xfm.mBasisZ.x = -xfm.mBasisX.z;
        xfm.mBasisZ.z = xfm.mBasisX.x;
        xfm.mBasisZ.y = 0.0f;
        flAngle -= flHalfRing;
        xfm.mTranslation.x = mUnknown38 * sinf(flAngle);
        xfm.mTranslation.y = 0.0f;
        xfm.mTranslation.z = -mUnknown38 * cosf(flAngle);
        flAngle += flHalfRing * 3.0f;
    }

    for (int nRing = 0; nRing < mRingCount; ++nRing) {
        LaneProfile &profile = mLaneProfiles[nRing];
        LerpRingSectionTangent(
            WrapIndex(nRing - 1, mRingCount), &profile.mPoints[0], 1.0f - mUnknown4c);
        LerpRingSectionTangent(nRing, &profile.mPoints[3], mUnknown4c);
        Vector3 center;
        center.w = 1.0f;
        LerpRingSectionTangent(nRing, &center, 0.0f);
        Vector3 floorCenter;
        floorCenter.w = 1.0f;
        Vec3Scale(&center.x, 1.0f - mUnknown48, &floorCenter.x);
        BlendVector(
            profile.mPoints[0], mUnknown50, floorCenter, 1.0f - mUnknown50, profile.mPoints[1]);
        BlendVector(
            profile.mPoints[3], mUnknown50, floorCenter, 1.0f - mUnknown50, profile.mPoints[2]);
        for (int nSegment = 0; nSegment < kLaneSegmentCount; ++nSegment) {
            Vector3 delta;
            delta.w = 1.0f;
            Vec3Sub(&profile.mPoints[nSegment + 1].x, &profile.mPoints[nSegment].x, &delta.x);
            Vector3 &normal = profile.mNormals[nSegment];
            normal.x = -delta.z;
            normal.y = 0.0f;
            normal.z = delta.x;
            normal.w = 1.0f;
            Vec3Normalize(&normal.x, &normal.x);
        }
    }

    mSliceSteps = 1 << (mLodCount - 1);
    mCellEdgeBlend = mUnknown54 * mSliceSteps;
    BuildSliceMeshes();
    BuildCellMeshes();
    // Yes, the binary passes the member to its own setter, which assigns it to itself.
    ApplyMeshLodScreenSizes(mLodScreenSizes);

    for (unsigned i = 0; i < mUnknowna4.size(); ++i) {
        if (i < cellMats.size()) {
            mUnknowna4[i].front()->SetMaterialChain(cellMats[i]);
            mUnknowna4[i].front()->SetVertexColor(cellColors[i]);
        }
    }
    for (unsigned i = 0; i < mUnknownb0.size(); ++i) {
        if (i < sliceMats.size()) {
            mUnknownb0[i].front()->SetMaterialChain(sliceMats[i]);
            mUnknownb0[i].front()->SetVertexColor(sliceColors[i]);
        }
    }
}

// 0x0046adc0
void Tunnel::BuildSliceMeshes() {
    mUnknownb0.resize(mSliceCount, TunnelMeshChain());
    if (mUnknownb0.empty()) {
        return;
    }

    const int nColumns = mSliceSteps + 1;
    const int nPanelVerts = kPanelRows * nColumns;
    const int nCapStart = kLaneSegmentCount * nPanelVerts;
    const int nBlockVerts = nCapStart + 2 * kCapVerts;
    const Color white{1.0f, 1.0f, 1.0f, 1.0f};
    for (unsigned nSlice = 0; nSlice < mUnknownb0.size(); ++nSlice) {
        RunLongOperationDrawProc();
        TunnelMeshChain &chain = mUnknownb0[nSlice];
        chain.Build(HxStr(FormatString(kSliceNameFormat, NameText(this), nSlice)), mLodCount, true);
        chain.SetVertexCount(nBlockVerts * mRingCount);
        Mesh *pMesh = chain.front();
        std::vector<MeshVert> &verts = pMesh->mVertsOwner->mVerts;
        for (int nRing = 0; nRing < mRingCount; ++nRing) {
            const int nBase = nRing * nBlockVerts;
            for (int nPanel = 0; nPanel < kLaneSegmentCount; ++nPanel) {
                SetGridTexCoords(verts, nBase + nPanel * nPanelVerts, kPanelRows, nColumns, 1.0f);
            }
            SetCapTexCoords(verts, nBase + nCapStart, 0.0f, 1.0f);
            SetCapTexCoords(verts, nBase + nCapStart + kCapVerts, 0.0f, 1.0f);
        }
        pMesh->SetVertexColor(white);
    }

    // The triangles are built once, on the chain of the first slice.
    TunnelMeshChain &first = mUnknownb0.front();
    for (unsigned nLevel = 0; nLevel < first.size(); ++nLevel) {
        RunLongOperationDrawProc();
        const int nStep = 1 << nLevel;
        Mesh *pMesh = first[nLevel];
        for (int nRing = 0; nRing < mRingCount; ++nRing) {
            const int nBase = nRing * nBlockVerts;
            if (nLevel == first.size() - 1) {
                // The coarsest level spans the lane with one strip, from the first row of the left
                // wall to the second row of the right wall.
                const int nRightWall = nBase + kLanePanelRightWall * nPanelVerts;
                pMesh->AddQuadStrip(nBase, nRightWall + nColumns, nColumns, nStep);
            } else {
                for (int nPanel = 0; nPanel < kLaneSegmentCount; ++nPanel) {
                    const int nPanelStart = nBase + nPanel * nPanelVerts;
                    pMesh->AddQuadStrip(nPanelStart, nPanelStart + nColumns, nColumns, nStep);
                }
            }
            const int nCap = nBase + nCapStart;
            pMesh->AddQuad(nCap, nCap + 1, nCap + 2, nCap + 3);
            pMesh->AddQuad(
                nCap + kCapVerts, nCap + kCapVerts + 1, nCap + kCapVerts + 2, nCap + kCapVerts + 3);
        }
    }
    first.Sync();
    for (unsigned nSlice = 1; nSlice < mUnknownb0.size(); ++nSlice) {
        RunLongOperationDrawProc();
        mUnknownb0[nSlice].ShareFaces(first);
        // Yes, the binary synchronises every level a second time.
        mUnknownb0[nSlice].Sync();
    }
}

// 0x0046b830
void Tunnel::BuildLaneMeshes() {
    mUnknownb0.resize(mRingCount * mSliceCount, TunnelMeshChain());
    if (mUnknownb0.empty()) {
        return;
    }

    const int nColumns = mSliceSteps + 1;
    const int nCapStart = kFlatLaneRows * nColumns;
    const int nBlockVerts = nCapStart + 2 * kCapVerts;
    const Color white{1.0f, 1.0f, 1.0f, 1.0f};
    for (unsigned nLane = 0; nLane < mUnknownb0.size(); ++nLane) {
        TunnelMeshChain &chain = mUnknownb0[nLane];
        chain.Build(HxStr(FormatString(kSliceNameFormat, NameText(this), nLane)), mLodCount, true);
        chain.SetVertexCount(nBlockVerts);
        Mesh *pMesh = chain.front();
        std::vector<MeshVert> &verts = pMesh->mVertsOwner->mVerts;
        SetGridTexCoords(verts, 0, kFlatLaneRows, nColumns, 1.0f / (kFlatLaneRows - 1));
        SetCapTexCoords(verts, nCapStart, kFlatLaneCapFirstU, kFlatLaneCapStepU);
        SetCapTexCoords(verts, nCapStart + kCapVerts, kFlatLaneCap2FirstU, kFlatLaneCap2StepU);
        pMesh->SetVertexColor(white);
    }

    TunnelMeshChain &first = mUnknownb0.front();
    for (unsigned nLevel = 0; nLevel < first.size(); ++nLevel) {
        const int nStep = 1 << nLevel;
        Mesh *pMesh = first[nLevel];
        if (nLevel == first.size() - 1) {
            pMesh->AddQuadStrip(0, (kFlatLaneRows - 1) * nColumns, nColumns, nStep);
        } else {
            for (int nRow = 0; nRow < kFlatLaneRows - 1; ++nRow) {
                pMesh->AddQuadStrip(nRow * nColumns, (nRow + 1) * nColumns, nColumns, nStep);
            }
        }
        pMesh->AddQuad(nCapStart, nCapStart + 1, nCapStart + 2, nCapStart + 3);
        pMesh->AddQuad(nCapStart + kCapVerts,
                       nCapStart + kCapVerts + 1,
                       nCapStart + kCapVerts + 2,
                       nCapStart + kCapVerts + 3);
    }
    first.Sync();
    for (unsigned nLane = 1; nLane < mUnknownb0.size(); ++nLane) {
        mUnknownb0[nLane].ShareFaces(first);
        // Yes, the binary synchronises every level a second time.
        mUnknownb0[nLane].Sync();
    }
}

// 0x0046c0e8
void Tunnel::BuildCellMeshes() {
    mUnknowna4.resize(mRingCount * mSliceCount, TunnelMeshChain());
    if (mUnknowna4.empty()) {
        return;
    }

    const int nColumns = mSliceSteps + 1;
    const Color white{1.0f, 1.0f, 1.0f, 1.0f};
    for (unsigned nCell = 0; nCell < mUnknowna4.size(); ++nCell) {
        RunLongOperationDrawProc();
        TunnelMeshChain &chain = mUnknowna4[nCell];
        chain.Build(HxStr(FormatString(kCellNameFormat, NameText(this), nCell)), mLodCount, true);
        chain.SetVertexCount(kPanelRows * nColumns);
        Mesh *pMesh = chain.front();
        SetGridTexCoords(pMesh->mVertsOwner->mVerts, 0, kPanelRows, nColumns, 1.0f);
        pMesh->SetVertexColor(white);
    }

    // The triangles are built once, on the chain of the first cell.
    TunnelMeshChain &first = mUnknowna4.front();
    for (unsigned nLevel = 0; nLevel < first.size(); ++nLevel) {
        RunLongOperationDrawProc();
        first[nLevel]->AddQuadStrip(0, nColumns, nColumns, 1 << nLevel);
    }
    for (unsigned nCell = 1; nCell < mUnknowna4.size(); ++nCell) {
        RunLongOperationDrawProc();
        mUnknowna4[nCell].ShareFaces(first);
    }
}

// 0x0046c638
void Tunnel::SetRingSectionFrames() {
    Transform xfm;
    SetPaddingWords(xfm);
    GetPathXfm(&xfm, mUnknown80);

    const int nColumns = mSliceSteps + 1;
    const int nPanelVerts = kPanelRows * nColumns;
    const int nCapStart = kLaneSegmentCount * nPanelVerts;
    const int nBlockVerts = nCapStart + 2 * kCapVerts;
    for (int nRing = 0; nRing < mRingCount; ++nRing) {
        const int nBase = nRing * nBlockVerts;
        Mesh *pSlice = GetRingSection(mUnknown7c);
        std::vector<MeshVert> &verts = pSlice->mVertsOwner->mVerts;
        const LaneProfile &profile = mLaneProfiles[nRing];

        // Column mUnknown84 of the two rows of each panel.
        const int nLeft = nBase + kLanePanelLeftWall * nPanelVerts + mUnknown84;
        const int nFloor = nBase + kLanePanelFloor * nPanelVerts + mUnknown84;
        const int nRight = nBase + kLanePanelRightWall * nPanelVerts + mUnknown84;
        XfmPoint(xfm, profile.mPoints[0], verts[nLeft].mPoint);
        XfmPoint(xfm, profile.mPoints[1], verts[nLeft + nColumns].mPoint);
        XfmPoint(xfm, profile.mPoints[2], verts[nRight].mPoint);
        XfmPoint(xfm, profile.mPoints[3], verts[nRight + nColumns].mPoint);
        verts[nFloor + nColumns].mPoint = verts[nRight].mPoint;
        verts[nFloor].mPoint = verts[nLeft + nColumns].mPoint;
        XfmVector(xfm, profile.mNormals[0], verts[nLeft].mNorm);
        XfmVector(xfm, profile.mNormals[1], verts[nFloor].mNorm);
        XfmVector(xfm, profile.mNormals[2], verts[nRight].mNorm);
        verts[nLeft + nColumns].mNorm = verts[nLeft].mNorm;
        verts[nFloor + nColumns].mNorm = verts[nFloor].mNorm;
        verts[nRight + nColumns].mNorm = verts[nRight].mNorm;

        // The cell to the right of the lane starts at its right edge, and the cell to its left
        // ends at its left edge.
        Mesh *pCell = GetRingSection(nRing, mUnknown7c);
        Mesh *pPreviousCell = GetRingSection(WrapIndex(nRing - 1, mRingCount), mUnknown7c);
        std::vector<MeshVert> &cellVerts = pCell->mVertsOwner->mVerts;
        std::vector<MeshVert> &previousVerts = pPreviousCell->mVertsOwner->mVerts;
        cellVerts[mUnknown84].mPoint = verts[nRight + nColumns].mPoint;
        previousVerts[mUnknown84 + nColumns].mPoint = verts[nLeft].mPoint;

        const Vector3 &floorNormal = verts[nFloor].mNorm;
        const int nCap = nBase + nCapStart;
        const int nPreviousCap = WrapIndex(nBase - nBlockVerts, verts.size()) + nCapStart;
        if (mUnknown84 == 0) {
            verts[nCap].mPoint = verts[nRight + nColumns].mPoint;
            verts[nPreviousCap + 2].mPoint = verts[nLeft].mPoint;
            BlendVector(cellVerts[1].mPoint,
                        mCellEdgeBlend,
                        cellVerts[0].mPoint,
                        1.0f - mCellEdgeBlend,
                        cellVerts[0].mPoint);
            BlendVector(previousVerts[nColumns + 1].mPoint,
                        mCellEdgeBlend,
                        previousVerts[nColumns].mPoint,
                        1.0f - mCellEdgeBlend,
                        previousVerts[nColumns].mPoint);
            verts[nCap + 1].mPoint = cellVerts[0].mPoint;
            verts[nPreviousCap + 3].mPoint = previousVerts[nColumns].mPoint;
            BlendVector(cellVerts[mSliceSteps - 1].mPoint,
                        mCellEdgeBlend,
                        cellVerts[mSliceSteps].mPoint,
                        1.0f - mCellEdgeBlend,
                        cellVerts[mSliceSteps].mPoint);
            BlendVector(previousVerts[nPanelVerts - 2].mPoint,
                        mCellEdgeBlend,
                        previousVerts[nPanelVerts - 1].mPoint,
                        1.0f - mCellEdgeBlend,
                        previousVerts[nPanelVerts - 1].mPoint);
            verts[nCap + kCapVerts].mPoint = cellVerts[mSliceSteps].mPoint;
            verts[nPreviousCap + kCapVerts + 2].mPoint = previousVerts[nPanelVerts - 1].mPoint;
            verts[nCap].mNorm = floorNormal;
            verts[nCap + 1].mNorm = floorNormal;
            verts[nPreviousCap + 2].mNorm = floorNormal;
            verts[nPreviousCap + 3].mNorm = floorNormal;
        } else if (mUnknown84 == mSliceSteps) {
            verts[nCap + kCapVerts + 1].mPoint = verts[nRight + nColumns].mPoint;
            verts[nPreviousCap + kCapVerts + 3].mPoint = verts[nLeft].mPoint;
            verts[nCap + kCapVerts].mNorm = floorNormal;
            verts[nCap + kCapVerts + 1].mNorm = floorNormal;
            verts[nPreviousCap + kCapVerts + 2].mNorm = floorNormal;
            verts[nPreviousCap + kCapVerts + 3].mNorm = floorNormal;
        }
    }

    if (mUnknown84 == 0) {
        GetRingSection(mUnknown7c)->SyncAll();
        for (int nRing = 0; nRing < mRingCount; ++nRing) {
            GetRingSection(nRing, mUnknown7c)->SyncAll();
        }
    }
}

// 0x0046d788
void Tunnel::SetLaneDividerColor(int nRing, int nSlice, const Color &color) {
    const int nColumns = mSliceSteps + 1;
    const int nPanelVerts = kPanelRows * nColumns;
    const int nCapStart = kLaneSegmentCount * nPanelVerts;
    const int nBlockVerts = nCapStart + 2 * kCapVerts;
    const int nBase = WrapIndex(nRing, mRingCount) * nBlockVerts;
    const int nNextBase = WrapIndex(nBase + nBlockVerts, nBlockVerts * mRingCount);
    Mesh *pMesh = mUnknownb0[WrapIndex(nSlice, mSliceCount)].front();
    std::vector<MeshVert> &verts = pMesh->mVertsOwner->mVerts;
    FillColor(verts, nBase + kLanePanelRightWall * nPanelVerts, nPanelVerts, color);
    FillColor(verts, nNextBase + kLanePanelLeftWall * nPanelVerts, nPanelVerts, color);
    FillColor(verts, nBase + nCapStart, kCapVerts, color);
    pMesh->SyncChanged(Mesh::kSyncColors);
}

// 0x0046d8e8
void Tunnel::SetLaneFloorColor(const Color &color) {
    const int nColumns = mSliceSteps + 1;
    const int nPanelVerts = kPanelRows * nColumns;
    const int nCapStart = kLaneSegmentCount * nPanelVerts;
    const int nBlockVerts = nCapStart + 2 * kCapVerts;
    for (int nSlice = 0; nSlice < mSliceCount; ++nSlice) {
        Mesh *pMesh = mUnknownb0[nSlice].front();
        std::vector<MeshVert> &verts = pMesh->mVertsOwner->mVerts;
        for (int nRing = 0; nRing < mRingCount; ++nRing) {
            // The image also tests nBlockVerts * mRingCount for zero with a divide trap here, and
            // uses no quotient.
            const int nBase = nRing * nBlockVerts;
            FillColor(verts, nBase + kLanePanelFloor * nPanelVerts, nPanelVerts, color);
            FillColor(verts, nBase + nCapStart + kCapVerts, kCapVerts, color);
        }
        pMesh->SyncChanged(Mesh::kSyncColors);
    }
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
    mLodScreenSizes = screenSizes;
    for (TunnelMeshChain &chain : mUnknowna4) {
        chain.SetScreenSizes(mLodScreenSizes);
    }
    for (TunnelMeshChain &chain : mUnknownb0) {
        chain.SetScreenSizes(mLodScreenSizes);
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

// 0x00477538
// A VU0 multiply and accumulate in the image, vmulax then vmaddx over xyz.
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
        mUnknown84 = mSliceSteps;
        mUnknown80 = nSlice * mSliceFrames;
    }
    SetRingSectionFrames();
    if (mUnknown84 == 0) {
        mUnknown88[nIndex] = nSlice;
        mUnknown7c = kNoSlice;
    } else {
        --mUnknown84;
        mUnknown80 += mSliceFrames / mSliceSteps;
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
    try {
        return new Tunnel(name);
    } catch (...) {
        return nullptr; // The binary's handler returns null.
    }
}

// 0x00476468
Object *CreateRegisteredTunnel(const HxStr &name) {
    try {
        return new Tunnel(name);
    } catch (...) {
        return nullptr;
    }
}

// 0x00894d64
int g_nTunnelLoadVersion;

} // namespace Rnd
