#include "rnd/psmultimesh.h"

#include "gfx/gfxdevice.h"
#include "gfx/renderstats.h"
#include "os/hxstr.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/psmat.h"
#include "rnd/psmesh.h"

namespace Rnd {

namespace {

// Quadwords reserved ahead of one material pass, the same budget Rnd::PsMesh::DrawSelf() reserves.
constexpr int kGifReserveQuadwords = 32;

} // namespace

// 0x005b5c28
MultiMesh *NewPsMultiMesh(const HxStr &name) {
    // The binary allocates the 0x38 bytes the object occupies.
    return new PsMultiMesh(name);
}

// 0x005b2fd8
PsMultiMesh::PsMultiMesh(const HxStr &name) : MultiMesh(name) {
}

// 0x005b5a40
PsMultiMesh::~PsMultiMesh() {
}

// 0x005b2ed0
int PsMultiMesh::DrawSelf() {
    ++g_renderStats.mnMeshDraws;
    if (g_gfxDevice.mnUseVu1 == 0) {
        return MultiMesh::DrawSelf();
    }

    if (mTransforms.empty()) {
        return 1;
    }
    if (mMesh == nullptr) {
        return 1;
    }
    if (mMesh->mFacesOwner->mFaces.size() == 0) {
        return 1;
    }

    Mat *pMat = mMesh->mMat;
    int nPass = 0;
    int nMorePasses = 0;
    do {
        g_gfxDevice.ReserveGifSpace(kGifReserveQuadwords);
        PsMesh::SelectDepthRegsForPass(*mMesh, nPass);
        if (pMat != nullptr) {
            // The narrowing is what psmat.h records as undecidable. Every material on this target
            // is a PsMat, because GfxDevice::Init() installs that creator.
            nMorePasses = static_cast<PsMat *>(pMat)->Select();
        } else {
            PsMat::SelectDefault();
        }
        SubmitInstanceGifPackets();
        ++nPass;
    } while (nMorePasses != 0);
    return 1;
}

} // namespace Rnd
