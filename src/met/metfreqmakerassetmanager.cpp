#include "met/metfreqmakerassetmanager.h"

#include <cstring>

#include "game/freqparttemplate.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/formatstring.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/tex.h"

namespace {

static const char *const kMeshNameFormat = "autogen_obj_%i";

// A part template stores its scale in 1/128 units.
constexpr float kTemplateScaleUnit = 1.0f / 128.0f;

// The basis rows ApplyPartScale() rebuilds and scales.
constexpr int kBasisRowCount = 3;
constexpr int kBasisXRow = 0;
constexpr int kBasisZRow = 2;

// Part categories count from 1.
constexpr int kFirstCategory = 1;

// The flags CloneMesh() copies the template mesh with.
constexpr unsigned kCloneCopyFlags = 0;

} // namespace

// 0x006a0f40
Color g_freqMakerDefaultColor = {0.75f, 0.75f, 0.75f, 1.0f};

// 0x006a0f60
HxStr g_spectrumTextureName("spectrum.bmp");

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

// 0x00254b30
void MetFreqMakerAssetManager::ApplyPartScale(Rnd::Mesh *pMesh,
                                              FreqPartTemplate *pTemplate,
                                              float *pScaleX,
                                              float *pScaleZ,
                                              float flFactorX,
                                              float flFactorZ) {
    *pScaleX = pTemplate->mScaleX * kTemplateScaleUnit;
    *pScaleZ = pTemplate->mScaleZ * kTemplateScaleUnit;

    float basis[kBasisRowCount][Rnd::kXfmRowFloatCount];
    std::memcpy(basis, pMesh->mLocalXfm, sizeof(basis));
    Mat33OrthonormalizeAroundY(&basis[0][0], &basis[0][0]);
    Vec3Scale(basis[kBasisXRow], *pScaleX * flFactorX, basis[kBasisXRow]);
    Vec3Scale(basis[kBasisZRow], *pScaleZ * flFactorZ, basis[kBasisZRow]);
    std::memcpy(pMesh->mLocalXfm, basis, sizeof(basis));
    pMesh->mDirty = 1;
}

// 0x00254f30
Color *MetFreqMakerAssetManager::ColorAt(const Vector2 &position) {
    if (mPaletteTex == nullptr) {
        mPaletteTex = dynamic_cast<Rnd::Tex *>(Rnd::g_manager.Find(g_spectrumTextureName));
    }
    return SampleTexture(mPaletteTex, position.x, position.y);
}

// 0x002549b8
FreqPartTemplate *MetFreqMakerAssetManager::FindPart(const HxStr &name) {
    PollLoad(); // Yes, the binary discards the result.
    std::map<HxStr, FreqPartTemplate *>::iterator it = mPartsByName.find(name);
    return it != mPartsByName.end() ? it->second : nullptr;
}

// 0x00254ea0
std::list<FreqPartTemplate *> *MetFreqMakerAssetManager::TemplatesInCategory(int nCategory) {
    // The binary dispatches through a jump table with one entry for each category.
    if (nCategory < kFirstCategory || nCategory > kCategoryCount) {
        return &mCategoryLists[kCategoryCount - 1];
    }
    return &mCategoryLists[nCategory - kFirstCategory];
}
