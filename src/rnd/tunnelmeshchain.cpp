#include "rnd/tunnelmeshchain.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/mesh.h"

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
            pMesh->mMinScreen = 0.0f;
            pMesh->SetNext(pCoarser);
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
        pMesh->mMinScreen = source[i]->mMinScreen;
        pMesh->SetNext(pMesh->mNext);
    }
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
