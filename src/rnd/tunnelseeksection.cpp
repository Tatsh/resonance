#include "rnd/tunnelseeksection.h"

#include <vector>

#include "math/color.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"
#include "rnd/tunnel.h"

namespace Rnd {

namespace {

constexpr float kFirstRowU = 0.0f;
constexpr float kSecondRowU = 1.0f;
constexpr float kMidV = 0.5f;
constexpr float kStartCapV = 0.0f;
constexpr float kEndCapV = 1.0f;

} // namespace

TunnelSeekSection::TunnelSeekSection() : mRing(-1), mSlice(kTunnelSeekNoSlice) {
}

void TunnelSeekSection::Set(int nSlice, int nRing, int bEndCap, int bStartCap) {
    mSlice = nSlice;
    mDirty = 1;
    mRing = nRing;
    mEndCap = bEndCap;
    mStartCap = bStartCap;
}

void TunnelSeekSection::Invalidate() {
    mSlice = kTunnelSeekNoSlice;
    mDirty = 1;
}

void TunnelSeekSection::Build(const HxStr &name, const LodMesh &templates) {
    mMeshes.Build(name, templates.size(), true);
    mMeshes.ShareFaces(templates);
    mMeshes.CopyScreenSizes(templates);
    mMeshes.front()->SetDepthChain(Mesh::kZModeZReadOnly, Mesh::kZFuncEqual);
}

void TunnelSeekSection::Update(Tunnel *pTunnel, const Color &color) {
    const int nSlice = Tunnel::WrapIndex(mSlice, pTunnel->mSliceCount);
    const int nRing = Tunnel::WrapIndex(mRing, pTunnel->mRingCount);
    const LodMesh &cell = pTunnel->mCellChains[nSlice * pTunnel->mRingCount + nRing];
    Mesh *pMesh = mMeshes.front();
    std::vector<MeshVert> &verts = pMesh->mVertsOwner->mVerts;
    verts = cell.front()->mVertsOwner->mVerts;
    pMesh->SetVertexColor(color);

    // Two rows of equal length. The first takes the horizontal coordinate 0 and the second 1.
    const int nRow = verts.size() / 2;
    for (int i = 0; i < nRow; ++i) {
        verts[i].mTex1.y = kMidV;
        verts[i].mTex1.x = kFirstRowU;
        verts[nRow + i].mTex1.y = kMidV;
        verts[nRow + i].mTex1.x = kSecondRowU;
    }
    if (mStartCap) {
        verts[nRow - 1].mTex1.y = kStartCapV;
        verts[nRow - 1].mTex1.x = kFirstRowU;
        verts[2 * nRow - 1].mTex1.x = kSecondRowU;
        verts[2 * nRow - 1].mTex1.y = kStartCapV;
    }
    if (mEndCap) {
        verts[0].mTex1.y = kEndCapV;
        verts[0].mTex1.x = kFirstRowU;
        verts[nRow].mTex1.y = kEndCapV;
        verts[nRow].mTex1.x = kSecondRowU;
    }
    pMesh->SyncAll();
    mDirty = 0;
}

} // namespace Rnd
