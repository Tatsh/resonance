#include "rnd/lodmesh.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"

namespace Rnd {

namespace {

constexpr char kInternalLevelFormat[] = "[%s.%d]";
constexpr char kLevelFormat[] = "%s.%d";

} // namespace

void LodMesh::DeleteMeshes() {
    for (Mesh *pMesh : *this) {
        if (pMesh != nullptr) {
            delete pMesh;
        }
    }
    erase(begin(), end());
}

void LodMesh::Build(const HxStr &name, int nCount, bool bInternal) {
    DeleteMeshes();
    resize(nCount);

    const char *pszName = name.mStr != nullptr ? name.mStr : g_szEmptyString;
    Mesh *pCoarser = nullptr;
    for (int nLevel = nCount - 1; nLevel >= 0; --nLevel) {
        Mesh *pMesh = NewMeshThroughHook(HxStr(
            Rnd::MakeString(bInternal ? kInternalLevelFormat : kLevelFormat, pszName, nLevel)));
        (*this)[nLevel] = pMesh;
        pMesh->mInternal = bInternal;
        pMesh->mZMode = Mesh::kZModeZReadWrite;
        pMesh->mZFunc = Mesh::kZFuncLess;
        if (pCoarser != nullptr) {
            pMesh->SetNext(pCoarser, 0.0f);
        }
        pCoarser = pMesh;
    }
    for (unsigned nLevel = 1; nLevel < size(); ++nLevel) {
        (*this)[nLevel]->SetVertsOwner(front());
    }
    SetTransOwner(front());
}

void LodMesh::SetTransOwner(Mesh *pOwner) {
    for (Mesh *pMesh : *this) {
        pMesh->SetTransOwner(pOwner);
    }
}

void LodMesh::CopyScreenSizes(const LodMesh &source) {
    for (unsigned i = 0; i < size(); ++i) {
        Mesh *pMesh = (*this)[i];
        pMesh->SetNext(pMesh->mNext, source[i]->mMinScreen);
    }
}

void LodMesh::SetScreenSizes(const std::vector<float> &screenSizes) {
    for (unsigned i = 0; i < size(); ++i) {
        if (i < screenSizes.size()) {
            Mesh *pMesh = (*this)[i];
            // Yes, the binary releases and immediately re-takes the reference on the same link.
            pMesh->SetNext(pMesh->mNext, screenSizes[i]);
        }
    }
}

void LodMesh::ShareFaces(const LodMesh &templates) {
    for (unsigned i = 0; i < size(); ++i) {
        (*this)[i]->SetFacesOwner(templates[i]->mFacesOwner);
        (*this)[i]->Sync();
    }
}

void LodMesh::Sync() {
    for (Mesh *pMesh : *this) {
        pMesh->Sync();
    }
}

void LodMesh::FindCollisions(const Segment &ray, std::list<Collideable::Collision> &collisions) {
    for (Mesh *pMesh : *this) {
        pMesh->FindCollisions(ray, collisions);
    }
}

void LodMesh::SetVertexCount(unsigned nCount) {
    MeshVert blank;
    blank.mPoint.x = 0.0f;
    blank.mPoint.y = 0.0f;
    blank.mPoint.z = 0.0f;
    blank.mPoint.w = 1.0f;
    blank.mNorm.x = 0.0f;
    blank.mNorm.y = 0.0f;
    blank.mNorm.z = 0.0f;
    blank.mNorm.w = 1.0f;
    blank.mColor.r = 1.0f;
    blank.mColor.g = 1.0f;
    blank.mColor.b = 1.0f;
    blank.mColor.a = 1.0f;
    blank.mTex1.x = 0.0f;
    blank.mTex1.y = 0.0f;
    blank.mTex2.x = 0.0f;
    blank.mTex2.y = 0.0f;
    front()->mVertsOwner->mVerts.resize(nCount, blank);
}

void LodMesh::Draw(float flScreenSize) {
    if (empty() || !front()->GetShowing()) {
        return;
    }
    iterator it = begin();
    while (it + 1 != end() && (*it)->mMinScreen < flScreenSize) {
        ++it;
    }
    (*it)->Draw();
}

} // namespace Rnd
