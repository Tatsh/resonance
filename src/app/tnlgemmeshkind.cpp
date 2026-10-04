#include "app/tnlgemmeshkind.h"

#include "rnd/drawable.h"
#include "rnd/mesh.h"
#include "rnd/multimesh.h"

TnlGemMeshKind::TnlGemMeshKind(Rnd::MultiMesh *pMesh, float flLodOffset, float flCostScale)
    : mMesh(pMesh), mNext(nullptr) {
    pMesh->GetTransforms().clear();
    Rnd::Mesh *pShape = pMesh->GetMesh();
    mLodDistance = pShape->mMinScreen + flLodOffset;
    mCost = pShape->mFacesOwner->mFaces.size() * flCostScale;
}

void TnlGemMeshKind::AddDrawTo(Rnd::Drawable *pParent) {
    if (pParent) {
        pParent->AddDraw(mMesh, nullptr);
        if (mNext) {
            mNext->AddDrawTo(pParent);
        }
    }
}

void TnlGemMeshKind::SetShowing(int nShowing) {
    mMesh->SetShowing(nShowing);
    if (mNext) {
        mNext->SetShowing(nShowing);
    }
}

float TnlGemMeshKind::GetCost() {
    return mMesh->GetShowing() ? mCost : 0.0f;
}
