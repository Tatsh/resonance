#include "rnd/tunnelmeshchain.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"

namespace Rnd {

namespace {

constexpr char kInternalLevelFormat[] = "[%s.%d]";
constexpr char kLevelFormat[] = "%s.%d";

} // namespace

// 0x00476b28
void TunnelMeshChain::DeleteMeshes() {
    for (Mesh *pMesh : *this) {
        if (pMesh != nullptr) {
            delete pMesh;
        }
    }
    erase(begin(), end());
}

// 0x004694a0
void TunnelMeshChain::Build(const HxStr &name, int nCount, bool bInternal) {
    DeleteMeshes();
    resize(nCount);

    const char *pszName = name.mStr != nullptr ? name.mStr : g_szEmptyString;
    Mesh *pCoarser = nullptr;
    for (int nLevel = nCount - 1; nLevel >= 0; --nLevel) {
        Mesh *pMesh = g_pfnNewMesh(
            HxStr(FormatString(bInternal ? kInternalLevelFormat : kLevelFormat, pszName, nLevel)));
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

// 0x00476e48
void TunnelMeshChain::SetTransOwner(Mesh *pOwner) {
    for (Mesh *pMesh : *this) {
        pMesh->SetTransOwner(pOwner);
    }
}

// 0x00469820
void TunnelMeshChain::CopyScreenSizes(const TunnelMeshChain &source) {
    for (unsigned i = 0; i < size(); ++i) {
        Mesh *pMesh = (*this)[i];
        pMesh->SetNext(pMesh->mNext, source[i]->mMinScreen);
    }
}

// 0x00476c50
void TunnelMeshChain::SetScreenSizes(const std::vector<float> &screenSizes) {
    for (unsigned i = 0; i < size(); ++i) {
        if (i < screenSizes.size()) {
            Mesh *pMesh = (*this)[i];
            // Yes, the binary releases and immediately re-takes the reference on the same link.
            pMesh->SetNext(pMesh->mNext, screenSizes[i]);
        }
    }
}

// 0x00476d28
void TunnelMeshChain::ShareFaces(const TunnelMeshChain &templates) {
    for (unsigned i = 0; i < size(); ++i) {
        (*this)[i]->SetFacesOwner(templates[i]->mFacesOwner);
        (*this)[i]->Sync();
    }
}

// 0x00476de8
void TunnelMeshChain::Sync() {
    for (Mesh *pMesh : *this) {
        pMesh->Sync();
    }
}

// 0x00476ec0
void TunnelMeshChain::Collide(const Ray &ray, Collideable::HitSink &sink) {
    for (Mesh *pMesh : *this) {
        pMesh->Collide(ray, sink);
    }
}

// 0x004698e8
void TunnelMeshChain::SetVertexCount(unsigned nCount) {
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

// 0x00476bc8
void TunnelMeshChain::Draw(float flScreenSize) {
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
