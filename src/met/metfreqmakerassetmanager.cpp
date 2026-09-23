#include "met/metfreqmakerassetmanager.h"

#include "os/formatstring.h"
#include "rnd/mesh.h"

namespace {

static const char *const kMeshNameFormat = "autogen_obj_%i";

// The flags CloneMesh() copies the template mesh with.
constexpr unsigned kCloneCopyFlags = 0;

} // namespace

// 0x006a0f40
Color g_freqMakerDefaultColor = {0.75f, 0.75f, 0.75f, 1.0f};

// 0x00254a18
FreqPartTemplate *MetFreqMakerAssetManager::GetPart(int nId) {
    PollLoad(); // Yes, the binary discards the result.
    return mParts[nId];
}

// 0x00254e50
HxStr MetFreqMakerAssetManager::NextMeshName() {
    return HxStr(FormatString(kMeshNameFormat, mMeshCount++));
}

// 0x00254a58
Rnd::Mesh *MetFreqMakerAssetManager::CloneMesh(const HxStr &name) {
    PollLoad(); // Yes, the binary discards the result.
    Rnd::Mesh *pMesh = Rnd::g_pfnNewMesh(name);
    pMesh->Copy(mMeshTemplate, kCloneCopyFlags);
    pMesh->SetVertexColor(g_freqMakerDefaultColor);
    return pMesh;
}
