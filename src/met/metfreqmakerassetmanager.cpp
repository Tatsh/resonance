#include "met/metfreqmakerassetmanager.h"

#include <cstring>

#include "game/freqparttemplate.h"
#include "game/globalsettings.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "met/metfreqloader.h"
#include "os/formatstring.h"
#include "os/hostmode.h"
#include "os/zone.h"
#include "rnd/asyncloader.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/object.h"
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

// The unit's static initialiser at 0x00254690 builds the globals below in this order.

// 0x006a0f40
Color g_freqMakerDefaultColor = {0.75f, 0.75f, 0.75f, 1.0f};

namespace {

// 0x006a0f50. The material RegisterPart() copies into every part material.
HxStr g_prototypeMaterialName("freq_maker_prototype_material");

// 0x006a0f58. The mesh CloneMesh() copies into every part mesh.
HxStr g_prototypeMeshName("freq_maker_prototype_mesh");

} // namespace

// 0x006a0f60
HxStr g_spectrumTextureName("spectrum.bmp");

namespace {

// 0x006a0f68. A loaded texture PollLoad() does not make a part of.
HxStr g_burnPrototypeName("persona_texburn_prototype_texture.tex");

// 0x00891ac8. The PC pre-fab persona file, which nothing reads.
HxStr g_prefabPersonaPathPc("metagame/persona/pre_fab_personas/pers_PC.dat");

// 0x00891ad0. The pre-fab persona file mPrefabLoader reads.
HxStr g_prefabPersonaPath("metagame/persona/pre_fab_personas/pers_PS2.dat");

// 0x00891ad8. The PC team FreQ persona file, which nothing reads.
HxStr g_teamFreqPersonaPathPc("metagame/persona/pre_fab_personas/teamfreq_pers_PC.dat");

// 0x00891ae0. The team FreQ persona file mTeamFreqLoader reads.
HxStr g_teamFreqPersonaPath("metagame/persona/pre_fab_personas/teamfreq_pers_PS2.dat");

// 0x00891ae8. A loaded texture PollLoad() does not make a part of.
HxStr g_freqFrameName("freq_frame.bmp");

// The four burn textures, which PollLoad() also does not make parts of.
static const char *const kBurnTexture1 = "persona_texburn_texture_1.tex";
static const char *const kBurnTexture2 = "persona_texburn_texture_2.tex";
static const char *const kBurnTexture3 = "persona_texburn_texture_3.tex";
static const char *const kBurnTexture4 = "persona_texburn_texture_4.tex";

// The asset file StartAssetLoad() queues, and the zone whose index it passes as the priority.
static const char *const kAssetZone = "rndglobal";
static const char *const kAssetDirectory = "MetaGame/persona/";
static const char *const kAssetFile = "freq_maker_inventory_assets.rnd";

// A part material is named after its texture with this appended.
static const char *const kMaterialSuffix = ".mat";

// The flags RegisterPart() copies the prototype material with.
constexpr unsigned kMaterialCopyFlags = 0;

// A part texture's name ends with its category letter, its colour flag, its randomisation flag,
// and a four-character extension, in that order.
constexpr int kCategoryFromEnd = 7;
constexpr int kColorableFromEnd = 6;
constexpr int kRandomizableFromEnd = 5;

// The categories the letters select.
enum PartCategory {
    kPartCategoryNone = 0,
    kPartCategoryB = 1,
    kPartCategoryH = 2,
    kPartCategoryD = 3,
    kPartCategoryE = 4,
    kPartCategoryN = 5,
    kPartCategoryM = 6,
    kPartCategoryL = 7,
    kPartCategoryR = 8,
    kPartCategoryA = 9,
    kPartCategoryC = 10,
    kPartCategoryS = 11
};

// Report the category a letter selects, in either case. The binary dispatches through a jump
// table over 'A' to 's'.
inline int CategoryOfLetter(char cLetter) {
    switch (cLetter) {
    case 'A':
    case 'a':
        return kPartCategoryA;
    case 'B':
    case 'b':
        return kPartCategoryB;
    case 'C':
    case 'c':
        return kPartCategoryC;
    case 'D':
    case 'd':
        return kPartCategoryD;
    case 'E':
    case 'e':
        return kPartCategoryE;
    case 'H':
    case 'h':
        return kPartCategoryH;
    case 'L':
    case 'l':
        return kPartCategoryL;
    case 'M':
    case 'm':
        return kPartCategoryM;
    case 'N':
    case 'n':
        return kPartCategoryN;
    case 'R':
    case 'r':
        return kPartCategoryR;
    case 'S':
    case 's':
        return kPartCategoryS;
    default:
        return kPartCategoryNone;
    }
}

} // namespace

// 0x00250b18
MetFreqMakerAssetManager::MetFreqMakerAssetManager()
    : mUnknown00(0), mPrefabLoader(nullptr), mTeamFreqLoader(nullptr), mAssetLoader(nullptr),
      mLoaded(0), mMaterialTemplate(nullptr), mMeshTemplate(nullptr), mPaletteTex(nullptr),
      mMeshCount(0) {
    HxStr teamFreqPath;
    HxStr prefabPath;
    prefabPath = GetFreqRoot() + g_prefabPersonaPath;
    teamFreqPath = GetFreqRoot() + g_teamFreqPersonaPath;
    mPrefabLoader = new MetFreqLoader(prefabPath, &mPrefabIdentities);
    mTeamFreqLoader = new MetFreqLoader(teamFreqPath, &mTeamFreqIdentities);
}

// 0x00250808
MetFreqMakerAssetManager::~MetFreqMakerAssetManager() {
    delete mPrefabLoader;
    delete mTeamFreqLoader;
}

// 0x0024f798
bool MetFreqMakerAssetManager::PollLoad() {
    if (mLoaded == 1) {
        return true;
    }

    StartAssetLoad();
    RndAsyncLoader::PollAsyncLoads();
    float flProgress;
    if (!mAssetLoader->Poll(&flProgress)) {
        return false;
    }

    mMaterialTemplate = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(g_prototypeMaterialName));
    mMeshTemplate = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(g_prototypeMeshName));
    std::list<Rnd::Object *> objects = GetLoadedObjects();
    HxStr unused; // Yes, the binary builds an empty string it never reads.

    mParts.erase(mParts.begin(), mParts.end());
    mParts.resize(objects.size());
    int nId = 0;
    for (std::list<Rnd::Object *>::iterator it = objects.begin(); it != objects.end(); ++it) {
        Rnd::Object *pObject = *it;
        const HxStr &name = pObject->mName;
        if (name == g_spectrumTextureName || name == g_burnPrototypeName ||
            name == g_freqFrameName || name == kBurnTexture1 || name == kBurnTexture2 ||
            name == kBurnTexture3 || name == kBurnTexture4) {
            continue;
        }

        FreqPartTemplate *pTemplate = RegisterPart(pObject);
        mPartsByName[name] = pTemplate;
        mParts[nId] = pTemplate;
        pTemplate->mId = nId;
        ++nId;
    }

    mLoaded = 1;
    StartIdentityLoads();
    return true;
}

// 0x0024fce8
void MetFreqMakerAssetManager::StartAssetLoad() {
    if (mAssetLoader != nullptr) {
        return;
    }
    int nZone = FindZoneByName(kAssetZone);
    mAssetLoader = new RndAsyncLoader(HxStr(kAssetDirectory), HxStr(kAssetFile), nZone);
    mAssetLoader->Enqueue();
}

// 0x00250540
std::list<Rnd::Object *> MetFreqMakerAssetManager::GetLoadedObjects() {
    PollLoad(); // Yes, the binary discards the result.
    return mAssetLoader->mObjects;
}

// 0x0024fe58
void MetFreqMakerAssetManager::ReleaseParts() {
    for (std::map<HxStr, FreqPartTemplate *>::iterator it = mPartsByName.begin();
         it != mPartsByName.end();
         ++it) {
        delete it->second;
        it->second = nullptr;
    }
    mPartsByName.clear();
    mLoaded = 0;
}

// 0x0024ff80
FreqPartTemplate *MetFreqMakerAssetManager::RegisterPart(Rnd::Object *pObject) {
    HxStr name;
    HxStr materialName;
    name = pObject->mName;
    materialName = name + kMaterialSuffix;

    Rnd::Mat *pMaterial = Rnd::g_pfnNewMat(materialName);
    pMaterial->Copy(mMaterialTemplate, kMaterialCopyFlags);
    Rnd::Tex *pTexture = static_cast<Rnd::Tex *>(pObject);
    pMaterial->mStages[0].SetTex(pTexture);
    int nWidth;
    int nHeight;
    int nBitsPerPixel;
    int nBytes;
    pTexture->GetBitmapInfo(nWidth, nHeight, nBitsPerPixel, nBytes);
    FreqPartTemplate *pTemplate = new FreqPartTemplate(name, pMaterial, nWidth, nHeight);

    const char *pszEnd = (name.mStr != nullptr ? name.mStr : g_szEmptyString) + name.mLen;
    char cCategory = pszEnd[-kCategoryFromEnd];
    char cRandomizable = pszEnd[-kRandomizableFromEnd];
    char cColorable = pszEnd[-kColorableFromEnd];
    int nCategory = CategoryOfLetter(cCategory);
    if (nCategory != kPartCategoryNone) {
        pTemplate->mCategory = nCategory;
        mCategoryLists[nCategory - kFirstCategory].push_back(pTemplate);
    }
    pTemplate->mColorable = cColorable == 'c' || cColorable == 'C';
    pTemplate->mRandomizable = cRandomizable == 'r' || cRandomizable == 'R';
    return pTemplate;
}

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

// 0x00255090
std::vector<MetPersonaData *> *MetFreqMakerAssetManager::GetIdentityList() {
    WaitForLoad();
    while (!AreIdentitiesLoaded()) {
    }
    return GlobalSettings::shared()->mTeamFreqUnlocked != 0 ? &mTeamFreqIdentities :
                                                              &mPrefabIdentities;
}

// 0x00255100
std::vector<MetPersonaData *> *MetFreqMakerAssetManager::GetAllIdentities() {
    WaitForLoad();
    while (!AreIdentitiesLoaded()) {
    }
    return &mTeamFreqIdentities;
}

// 0x00254fd0
void MetFreqMakerAssetManager::StartIdentityLoads() {
    mPrefabLoader->Start();
    mTeamFreqLoader->Start();
}

// 0x00255000
bool MetFreqMakerAssetManager::AreIdentitiesLoaded() {
    int nPrefabLoaded = mPrefabLoader->IsLoaded();
    int nTeamFreqLoaded = mTeamFreqLoader->IsLoaded();
    return nTeamFreqLoaded != 0 && nPrefabLoaded != 0;
}

// 0x00255048
bool MetFreqMakerAssetManager::AreLoadersReady() {
    bool bPrefabReady = mPrefabLoader->PollAssets();
    bool bTeamFreqReady = mTeamFreqLoader->PollAssets();
    return bTeamFreqReady && bPrefabReady;
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
