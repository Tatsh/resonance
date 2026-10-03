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

// NTSC-U/C: 0x00476b28, PAL: 0x004b47a0
void LodMesh::DeleteMeshes() {
    for (Mesh *pMesh : *this) {
        if (pMesh != nullptr) {
            delete pMesh;
        }
    }
    erase(begin(), end());
}

// NTSC-U/C: 0x004694a0, PAL: 0x004a6f60
void LodMesh::Build(const HxStr &name, int nCount, bool bInternal) {
    DeleteMeshes();
    resize(nCount);

    const char *pszName = name.mStr != nullptr ? name.mStr : g_szEmptyString;
    Mesh *pCoarser = nullptr;
    for (int nLevel = nCount - 1; nLevel >= 0; --nLevel) {
        Mesh *pMesh = NewMeshThroughHook(
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

// NTSC-U/C: 0x00476e48, PAL: 0x004b4ac0
void LodMesh::SetTransOwner(Mesh *pOwner) {
    for (Mesh *pMesh : *this) {
        pMesh->SetTransOwner(pOwner);
    }
}

// NTSC-U/C: 0x00469820, PAL: 0x004a7318
void LodMesh::CopyScreenSizes(const LodMesh &source) {
    for (unsigned i = 0; i < size(); ++i) {
        Mesh *pMesh = (*this)[i];
        pMesh->SetNext(pMesh->mNext, source[i]->mMinScreen);
    }
}

// NTSC-U/C: 0x00476c50, PAL: 0x004b48c8
void LodMesh::SetScreenSizes(const std::vector<float> &screenSizes) {
    for (unsigned i = 0; i < size(); ++i) {
        if (i < screenSizes.size()) {
            Mesh *pMesh = (*this)[i];
            // Yes, the binary releases and immediately re-takes the reference on the same link.
            pMesh->SetNext(pMesh->mNext, screenSizes[i]);
        }
    }
}

// NTSC-U/C: 0x00476d28, PAL: 0x004b49a0
void LodMesh::ShareFaces(const LodMesh &templates) {
    for (unsigned i = 0; i < size(); ++i) {
        (*this)[i]->SetFacesOwner(templates[i]->mFacesOwner);
        (*this)[i]->Sync();
    }
}

// NTSC-U/C: 0x00476de8, PAL: 0x004b4a60
void LodMesh::Sync() {
    for (Mesh *pMesh : *this) {
        pMesh->Sync();
    }
}

// NTSC-U/C: 0x00476ec0, PAL: 0x004b4b38
void LodMesh::Collide(const Ray &ray, Collideable::HitSink &sink) {
    for (Mesh *pMesh : *this) {
        pMesh->Collide(ray, sink);
    }
}

// NTSC-U/C: 0x004698e8, PAL: 0x004a73e0
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

// NTSC-U/C: 0x00476bc8, PAL: 0x004b4840
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
