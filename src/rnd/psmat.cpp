#include "rnd/psmat.h"

#include "gfx/gfxdevice.h"
#include "gfx/renderstats.h"
#include "math/color.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/mat.h"
#include "rnd/tex.h"

namespace Rnd {

namespace {

// GS general-purpose register indices. The shipped build took these from the PlayStation 2 SDK
// headers, which this tree does not reproduce.
constexpr int kGsRegClamp1 = 0x08;
constexpr int kGsRegAlpha1 = 0x42;
constexpr int kGsRegDimx = 0x44;
constexpr int kGsRegTest1 = 0x47;
constexpr int kGsRegFba1 = 0x4a;

// ALPHA_1 equations, as the A, B, C, and D selector pairs of the register. Each name states the
// result the selectors produce, where Cs is the source colour, Cd the destination colour, As the
// source alpha, Ad the destination alpha, and FIX the fixed term in the top byte.
constexpr unsigned long long kAlphaKeepDest = 0x4a;
constexpr unsigned long long kAlphaAddFixed = 0x8000000068ULL;
constexpr unsigned long long kAlphaSrcAlphaAdd = 0x48;
constexpr unsigned long long kAlphaSrcAlpha = 0x44;
constexpr unsigned long long kAlphaInvSrcAlpha = 0x01;
constexpr unsigned long long kAlphaDestAlpha = 0x54;
constexpr unsigned long long kAlphaInvDestAlpha = 0x11;
constexpr unsigned long long kAlphaSrcAlphaOpaque = 0x88;

// The four selector pairs alone, and the same four plus the fixed term.
constexpr unsigned long long kAlphaSelectorMask = 0xff;
constexpr unsigned long long kAlphaSelectorAndFixedMask = 0xff000000ffULL;

// The two 4x4 dither matrices the blend modes choose between. The first has a shorter range and
// serves the modes that preserve or add to the destination.
constexpr unsigned long long kDimxGentle = 0x1212303012120303ULL;
constexpr unsigned long long kDimxStandard = 0x2435607135247160ULL;
constexpr unsigned long long kDimxMask = ~0ULL;

// TEST_1 with the alpha test enabled, greater than a reference of zero, which discards a fully
// transparent pixel. The first value also sets the failing pixel to update the depth buffer alone.
constexpr unsigned long long kTestDiscardClearZOnly = 0x200d;
constexpr unsigned long long kTestDiscardClear = 0x0d;
constexpr unsigned long long kTestAlphaAndFailMask = 0x3fff;
constexpr unsigned long long kTestAlphaEnableMask = 0x01;

// FBA_1 alpha correction.
constexpr unsigned long long kFbaMask = 0x01;

// CLAMP_1 with both axes clamped, and with both axes repeating.
constexpr unsigned long long kClampBothAxes = 0x05;
constexpr unsigned long long kRepeatBothAxes = 0x00;
constexpr unsigned long long kClampModeMask = 0x0f;

// mMultiPass value that makes a later stage modulate rather than replace.
constexpr int kMultiPassModulate = 2;

// 0x0076d688. Rnd::Mat::BlendMode to GS TEX0.TFX, where 0 is MODULATE, 1 DECAL, 2 HIGHLIGHT, and
// 3 HIGHLIGHT2. Every mode this table does not distinguish modulates.
int g_anStageBlendTexFunc[] = {0, 1, 3, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0};

// 0x0076d65c
int g_nSelectedStage;

// 0x0076d680. Set by PsMat::SelectAlphaBlend() and cleared once PsMat::Select() has restored the
// material blend.
int g_nBlendOverridden;

// 0x008e41c0. The identity halved with V flipped and the translation moved to the texture centre,
// which is the transform a sphere map needs.
Transform g_sphereMapUvXfm;

// 0x008e4200. Scratch the non-sphere path composes a stage transform into.
Transform g_stageUvXfm;

} // namespace

// 0x0076d658
Mat *g_pSelectedMat;

// 0x0076d660
int g_nSelectedGenMode;

// 0x0076d664
int g_nSelectedFlat;

// 0x0076d668
int g_nStageTextureBound;

// 0x0076d66c
int g_nAlphaBlendEnabled;

// 0x0076d670
int g_nStageBlendDoubles;

// 0x0076d674
Transform *g_pSelectedUvXfm;

// 0x0076d678
Transform *g_pSelectedStageXfm;

// 0x0076d67c
int g_nLightingEnabled;

// The constructor body is empty. 0x005914b8 inlines the whole of it.
PsMat::PsMat(const HxStr &name) : Mat(name) {
}

// 0x00591590
PsMat::~PsMat() {
    if (g_pSelectedMat == this) {
        g_pSelectedMat = nullptr;
    }
}

// 0x00591240
void PsMat::InstallCreator() {
    g_pfnNewMat = NewPsMat;
    g_pSelectedMat = nullptr;
    // The emitted code writes an identity and then the four values after it, which is an inlined
    // transform reset followed by the four assignments the source made.
    g_sphereMapUvXfm.mBasisX.x = 1.0f;
    g_sphereMapUvXfm.mBasisX.y = 0.0f;
    g_sphereMapUvXfm.mBasisX.z = 0.0f;
    g_sphereMapUvXfm.mBasisY.x = 0.0f;
    g_sphereMapUvXfm.mBasisY.y = 1.0f;
    g_sphereMapUvXfm.mBasisY.z = 0.0f;
    g_sphereMapUvXfm.mBasisZ.x = 0.0f;
    g_sphereMapUvXfm.mBasisZ.y = 0.0f;
    g_sphereMapUvXfm.mBasisZ.z = 1.0f;
    g_sphereMapUvXfm.mTranslation.x = 0.0f;
    g_sphereMapUvXfm.mTranslation.y = 0.0f;
    g_sphereMapUvXfm.mTranslation.z = 0.0f;
    g_sphereMapUvXfm.mTranslation.w = 1.0f;
    g_sphereMapUvXfm.mBasisX.x = 0.5f;
    g_sphereMapUvXfm.mBasisY.y = -0.5f;
    g_sphereMapUvXfm.mTranslation.x = 0.5f;
    g_sphereMapUvXfm.mTranslation.y = 0.5f;
}

// 0x005916b0
void PsMat::SetAmbient(const Color &color) {
    mAmbient = color;
    g_pSelectedMat = nullptr;
}

// 0x005916c8
void PsMat::SetDiffuse(const Vector3 &rgb) {
    mDiffuse.r = rgb.x;
    mDiffuse.g = rgb.y;
    mDiffuse.b = rgb.z;
    g_pSelectedMat = nullptr;
}

// 0x005916f0
void PsMat::SetEmissive(const Color &color) {
    mEmissive = color;
    g_pSelectedMat = nullptr;
}

// 0x00591730
void PsMat::SetAlpha(float flAlpha) {
    mDiffuse.a = flAlpha;
    g_pSelectedMat = nullptr;
}

// 0x00591708
void PsMat::SetSpecular(const Vector3 &rgb, float flAlpha) {
    mSpecular.r = rgb.x;
    mSpecular.g = rgb.y;
    mSpecular.b = rgb.z;
    mSpecular.a = flAlpha;
    g_pSelectedMat = nullptr;
}

// 0x005914b8
Mat *NewPsMat(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Mat" and rounds the object up to 0xa0
    // bytes.
    return new PsMat(name);
}

// 0x0058eb40
int PsMat::Select() {
    if (this == g_pSelectedMat && mStages.size() < 2) {
        if (g_nBlendOverridden != 0) {
            g_nSelectedStage = 0;
            SelectBlendMode();
            g_nBlendOverridden = 0;
        }
        return 0;
    }
    ++g_renderStats.mnMatSelects;
    if (this != g_pSelectedMat || g_nSelectedStage >= static_cast<int>(mStages.size())) {
        g_nSelectedStage = 0;
    }
    g_nSelectedFlat = mFlat;
    SelectBlendMode();
    BindStageTexture();
    SetupUvXfm();
    g_pSelectedMat = this;
    ++g_nSelectedStage;
    return g_nSelectedStage < static_cast<int>(mStages.size());
}

// 0x005910b0
void PsMat::SelectDefault() {
    ++g_renderStats.mnMatSelects;
    g_nAlphaBlendEnabled = 1;
    g_nStageTextureBound = 0;
    g_pSelectedMat = nullptr;
    g_nSelectedFlat = 0;
    g_nLightingEnabled = 0;
    g_nBlendOverridden = 0;
    g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaSrcAlpha, kAlphaSelectorMask);
    g_gfxDevice.SetGsReg(kGsRegFba1, 1, kFbaMask);
    g_gfxDevice.SetGsReg(kGsRegDimx, kDimxStandard, kDimxMask);
}

// 0x00591170
void PsMat::SelectAlphaBlend() {
    if (this == g_pSelectedMat && g_nAlphaBlendEnabled != 0) {
        return;
    }
    ++g_renderStats.mnMatSelects;
    g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaSrcAlpha, kAlphaSelectorMask);
    g_nBlendOverridden = 1;
}

// 0x0058ec80
void PsMat::SelectBlendMode() {
    BlendMode nBlend = mBlend;
    if (g_nSelectedStage != 0) {
        nBlend = mStages[g_nSelectedStage].mBlend;
    }
    g_gfxDevice.SetGsReg(kGsRegFba1, nBlend != kBlendModeSrc, kFbaMask);
    switch (nBlend) {
    case kBlendModeDest:
        g_nAlphaBlendEnabled = 1;
        g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaKeepDest, kAlphaSelectorMask);
        break;
    case kBlendModeAdd:
        g_nAlphaBlendEnabled = 1;
        g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaAddFixed, kAlphaSelectorAndFixedMask);
        break;
    case kBlendModeSrcAlphaAdd:
        g_nAlphaBlendEnabled = 1;
        g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaSrcAlphaAdd, kAlphaSelectorMask);
        break;
    case kBlendModeSrcAlpha:
    case kBlendModeSrcAlphaCutout:
        g_nAlphaBlendEnabled = 1;
        g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaSrcAlpha, kAlphaSelectorMask);
        break;
    case kBlendModeInvSrcAlpha:
        g_nAlphaBlendEnabled = 1;
        g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaInvSrcAlpha, kAlphaSelectorMask);
        break;
    case kBlendModeDestAlpha:
        g_nAlphaBlendEnabled = 1;
        g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaDestAlpha, kAlphaSelectorMask);
        break;
    case kBlendModeInvDestAlpha:
        g_nAlphaBlendEnabled = 1;
        g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaInvDestAlpha, kAlphaSelectorMask);
        break;
    case kBlendModeSrcAlphaOpaque:
        g_nAlphaBlendEnabled = 1;
        g_gfxDevice.SetGsReg(kGsRegAlpha1, kAlphaSrcAlphaOpaque, kAlphaSelectorMask);
        break;
    // Src, Multiply, Multiply2, and SrcAdd turn frame buffer blending off, because the texture
    // function performs the combine instead.
    case kBlendModeSrc:
    case kBlendModeMultiply:
    case kBlendModeMultiply2:
    case kBlendModeSrcAdd:
        g_nAlphaBlendEnabled = 0;
        break;
    }
    if (nBlend == kBlendModeDest || nBlend == kBlendModeAdd || nBlend == kBlendModeSrcAlphaAdd) {
        g_gfxDevice.SetGsReg(kGsRegDimx, kDimxGentle, kDimxMask);
    } else {
        g_gfxDevice.SetGsReg(kGsRegDimx, kDimxStandard, kDimxMask);
    }
    if (nBlend == kBlendModeSrcAlpha) {
        g_gfxDevice.SetGsReg(kGsRegTest1, kTestDiscardClearZOnly, kTestAlphaAndFailMask);
    } else if (nBlend == kBlendModeSrcAlphaCutout) {
        g_gfxDevice.SetGsReg(kGsRegTest1, kTestDiscardClear, kTestAlphaAndFailMask);
    } else {
        g_gfxDevice.SetGsReg(kGsRegTest1, 0, kTestAlphaEnableMask);
    }
}

// 0x0058eef8
void PsMat::BindStageTexture() {
    g_nLightingEnabled = mEnable;
    if (mStages.size() == 0) {
        g_nStageTextureBound = 0;
        return;
    }
    Stage &stage = mStages[g_nSelectedStage];
    if (stage.mTex == nullptr || stage.mBlend == kBlendModeDest) {
        g_nStageTextureBound = 0;
        return;
    }
    BlendMode nTexBlend = stage.mBlend;
    if (g_nSelectedStage != 0) {
        nTexBlend = mMultiPass == kMultiPassModulate ? kBlendModeMultiply : kBlendModeSrc;
    }
    if (!stage.mTex->BindToGsSlot(g_anStageBlendTexFunc[nTexBlend])) {
        g_nStageTextureBound = 0;
        return;
    }
    if (nTexBlend == kBlendModeSrc) {
        g_nLightingEnabled = 0;
    }
    g_nStageTextureBound = 1;
    g_nStageBlendDoubles = nTexBlend == kBlendModeMultiply2;
    SelectStageClamp(stage);
}

// 0x0058f038
void PsMat::SetupUvXfm() {
    if (g_nStageTextureBound == 0) {
        g_pSelectedUvXfm = nullptr;
        return;
    }
    Stage &stage = mStages[g_nSelectedStage];
    g_nSelectedGenMode = stage.mGenMode;
    if (stage.mGenMode == Stage::kGenModeSphere) {
        g_pSelectedUvXfm = &g_sphereMapUvXfm;
        g_pSelectedStageXfm = stage.mUseXfm != 0 ? &stage.mXfm : nullptr;
        return;
    }
    if (stage.mUseXfm == 0) {
        g_pSelectedUvXfm = nullptr;
        return;
    }
    // The V axis and the V translation invert, and both translations shift so that the transform
    // turns about the texture centre rather than its corner.
    constexpr float kTexCenter = 0.5f;
    const float flU = -stage.mXfm.mTranslation.x - kTexCenter;
    const float flV = stage.mXfm.mTranslation.y - kTexCenter;
    g_stageUvXfm.mBasisX.x = stage.mXfm.mBasisX.x;
    g_stageUvXfm.mBasisX.y = -stage.mXfm.mBasisX.y;
    g_stageUvXfm.mBasisY.x = -stage.mXfm.mBasisY.x;
    g_stageUvXfm.mBasisY.y = stage.mXfm.mBasisY.y;
    g_stageUvXfm.mTranslation.x =
        flU * g_stageUvXfm.mBasisX.x + flV * g_stageUvXfm.mBasisY.x + kTexCenter;
    g_stageUvXfm.mTranslation.y =
        flU * g_stageUvXfm.mBasisX.y + flV * g_stageUvXfm.mBasisY.y + kTexCenter;
    g_pSelectedUvXfm = &g_stageUvXfm;
}

// 0x005911d8. The compiler inlined this into BindStageTexture(), and nothing calls the out-of-line
// body, so it is dead in the shipped image. Its own receiver is unused, which is what identifies
// it as an instance method rather than a free function.
void PsMat::SelectStageClamp(const Stage &stage) {
    if (stage.mWrap == Stage::kWrapModeClamp) {
        g_gfxDevice.SetGsReg(kGsRegClamp1, kClampBothAxes, kClampModeMask);
    } else {
        g_gfxDevice.SetGsReg(kGsRegClamp1, kRepeatBothAxes, kClampModeMask);
    }
}

} // namespace Rnd
