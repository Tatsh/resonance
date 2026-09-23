#include "rnd/tunnelmeshchain.h"

#include "rnd/mesh.h"

namespace Rnd {

// 0x00476b28
void TunnelMeshChain::DeleteMeshes() {
    for (Mesh *pMesh : *this) {
        if (pMesh != nullptr) {
            delete pMesh;
        }
    }
    erase(begin(), end());
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

} // namespace Rnd
