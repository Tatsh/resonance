#include "met/metfreqmakerassetmanager.h"

#include <cstring>

#include "game/freqparttemplate.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/formatstring.h"
#include "rnd/asyncloader.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/tex.h"
#include "rndartt/abitmap.h"
#include "rndartt/acanvas.h"

namespace {

static const char *const kMeshNameFormat = "autogen_obj_%i";

// A part template stores its scale in 1/128 units.
constexpr float kTemplateScaleUnit = 1.0f / 128.0f;

// The basis rows ApplyPartScale() rebuilds and scales.
constexpr int kBasisRowCount = 3;
constexpr int kBasisXRow = 0;
constexpr int kBasisYRow = 1;
constexpr int kBasisZRow = 2;

// Part categories count from 1.
constexpr int kFirstCategory = 1;

// 0x006a0f30. The instance Create() allocates.
MetFreqMakerAssetManager *g_pFreqMakerAssetManager = nullptr;

// 0x006a0f70. The colour SampleTexture() reports.
Color g_sampledColor;

// The colour SampleTexture() clears the result to.
const Color kOpaqueBlack = {0.0f, 0.0f, 0.0f, 1.0f};

// The texture lock SampleTexture() and PaintTexel() take, on the top mip without a read-back.
constexpr int kTopMip = 0;
constexpr int kNoReadBack = 0;

// A texel is three bytes in red, green, and blue order.
enum TexelChannel { kTexelRed = 0, kTexelGreen = 1, kTexelBlue = 2, kTexelChannelCount = 3 };
constexpr int kOneTexel = 1;

// The scale between a byte channel and the unit range.
constexpr double kByteRange = 255.0;

// The value ScaleMesh() compares its flag against.
constexpr int kOrthonormalize = 1;

// The flags CloneMesh() copies the template mesh with.
constexpr unsigned kCloneCopyFlags = 0;

} // namespace

// 0x006a0f40
Color g_freqMakerDefaultColor = {0.75f, 0.75f, 0.75f, 1.0f};

// 0x006a0f60
HxStr g_spectrumTextureName("spectrum.bmp");

// 0x002551f0
MetFreqMakerAssetManager *MetFreqMakerAssetManager::shared() {
    return g_pFreqMakerAssetManager;
}

// 0x00255158
void MetFreqMakerAssetManager::Create() {
    g_pFreqMakerAssetManager = new MetFreqMakerAssetManager();
}

// 0x002551b8
void MetFreqMakerAssetManager::Destroy() {
    delete g_pFreqMakerAssetManager; // Yes, the binary does not clear the pointer.
}

// 0x00255200
void MetFreqMakerAssetManager::WaitForLoad() {
    while (!PollLoad()) {
        RndAsyncLoader::PollAsyncLoads();
    }
}

// 0x00254990
std::map<HxStr, FreqPartTemplate *> *MetFreqMakerAssetManager::GetPartsByName() {
    PollLoad(); // Yes, the binary discards the result.
    return &mPartsByName;
}

// 0x00250638
Color *MetFreqMakerAssetManager::SampleTexture(Rnd::Tex *pTex, float flU, float flV) {
    g_sampledColor = kOpaqueBlack;
    ACanvas *pCanvas = pTex->LockMipBitmap(kTopMip, 0, kNoReadBack);
    unsigned char abTexel[kTexelChannelCount];
    pCanvas->GetPixelRGB(static_cast<int>(flU * pCanvas->mBitmap.mWidth),
                         static_cast<int>(flV * pCanvas->mBitmap.mHeight),
                         abTexel);
    if (g_nSkipColorSwap == 0) {
        ABitmap::SwapRedBlue24(abTexel, kOneTexel);
    }

    g_sampledColor = kOpaqueBlack; // Yes, the binary clears the colour a second time.
    g_sampledColor.r = static_cast<float>(abTexel[kTexelRed] / kByteRange);
    g_sampledColor.g = static_cast<float>(abTexel[kTexelGreen] / kByteRange);
    g_sampledColor.b = static_cast<float>(abTexel[kTexelBlue] / kByteRange);
    pTex->UnlockMipBitmap();
    return &g_sampledColor;
}

// 0x00254cf8
bool MetFreqMakerAssetManager::PaintTexel(Rnd::Tex *pTex,
                                          const Color &color,
                                          float flU,
                                          float flV) {
    unsigned char abTexel[kTexelChannelCount];
    abTexel[kTexelRed] = static_cast<unsigned>(color.r * kByteRange);
    abTexel[kTexelGreen] = static_cast<unsigned>(color.g * kByteRange);
    abTexel[kTexelBlue] = static_cast<unsigned>(color.b * kByteRange);
    if (g_nSkipColorSwap == 0) {
        ABitmap::SwapRedBlue24(abTexel, kOneTexel);
    }

    ACanvas *pCanvas = pTex->LockMipBitmap(kTopMip, 0, kNoReadBack);
    pCanvas->PutPixelRGB(static_cast<int>(flU * pCanvas->mBitmap.mWidth),
                         static_cast<int>(flV * pCanvas->mBitmap.mHeight),
                         abTexel);
    pTex->UnlockMipBitmap();
    return true;
}

// 0x00254c20
bool MetFreqMakerAssetManager::ScaleMesh(
    Rnd::Mesh *pMesh, int nOrthonormalize, float flScaleX, float flScaleY, float flScaleZ) {
    float basis[kBasisRowCount][Rnd::kXfmRowFloatCount];
    std::memcpy(basis, pMesh->mLocalXfm, sizeof(basis));
    if (nOrthonormalize == kOrthonormalize) {
        Mat33OrthonormalizeAroundY(&basis[0][0], &basis[0][0]);
    }
    Vec3Scale(basis[kBasisXRow], flScaleX, basis[kBasisXRow]);
    Vec3Scale(basis[kBasisYRow], flScaleY, basis[kBasisYRow]);
    Vec3Scale(basis[kBasisZRow], flScaleZ, basis[kBasisZRow]);
    std::memcpy(pMesh->mLocalXfm, basis, sizeof(basis));
    pMesh->mDirty = 1;
    return true;
}

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
