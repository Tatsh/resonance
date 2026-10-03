#include "app/tnlgemmeshkind.h"

#include "rnd/drawable.h"
#include "rnd/mesh.h"
#include "rnd/multimesh.h"

// NTSC-U/C: 0x004158c0, PAL: 0x0044f410
TnlGemMeshKind::TnlGemMeshKind(Rnd::MultiMesh *pMesh, float flLodOffset, float flCostScale)
    : mMesh(pMesh), mNext(nullptr) {
    pMesh->GetTransforms().clear();
    Rnd::Mesh *pShape = pMesh->GetMesh();
    mLodDistance = pShape->mMinScreen + flLodOffset;
    mCost = pShape->mFacesOwner->mFaces.size() * flCostScale;
}

// NTSC-U/C: 0x00415980, PAL: 0x0044f4d0
void TnlGemMeshKind::AddDrawTo(Rnd::Drawable *pParent) {
    if (pParent) {
        pParent->AddDraw(mMesh, nullptr);
        if (mNext) {
            mNext->AddDrawTo(pParent);
        }
    }
}

// NTSC-U/C: 0x004159d8, PAL: 0x0044f528
void TnlGemMeshKind::SetShowing(int nShowing) {
    mMesh->SetShowing(nShowing);
    if (mNext) {
        mNext->SetShowing(nShowing);
    }
}

// NTSC-U/C: 0x00415a30, PAL: 0x0044f580
float TnlGemMeshKind::GetCost() {
    return mMesh->GetShowing() ? mCost : 0.0f;
}
