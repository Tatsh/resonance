#include "rnd/psenviron.h"

#include <vector>

#include "gfx/gfxdevice.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/drawverts.h"
#include "rnd/environ.h"
#include "rnd/light.h"
#include "rnd/pscam.h"

namespace Rnd {

namespace {

// The GS fog register takes a byte, and the fog terms map camera depth onto its range.
constexpr float kFogByteRange = 255.0f;

// FOGCOL and the 24 bits of it the colour occupies, one byte per component from red upwards.
constexpr int kGsRegFogCol = 0x3d;
constexpr unsigned long long kFogColMask = 0xffffff;
constexpr int kFogColGreenShift = 8;
constexpr int kFogColBlueShift = 16;

// Row of a world transform a directional light shines along, negated, and the translation row.
constexpr int kXfmRowLightAxis = 1;
constexpr int kXfmRowTranslation = 3;

} // namespace

// 0x00776118
int g_nFogEnabled;

// 0x00776120
std::vector<DirectionalLightRecord> g_directionalLightRecords;

// 0x00776130
std::vector<PointLightRecord> g_pointLightRecords;

// 0x0077613c
PsEnviron *g_pDefaultEnviron;

// 0x00776140
Light *g_pDefaultLight;

// 0x00776110
float g_flFogScale;

// 0x00776114
float g_flFogOffset;

// 0x005b2200
PsEnviron::~PsEnviron() {
}

// 0x005b27b0
Environ *PsEnviron::NewEnviron(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Environ" and the object is 0x80 bytes.
    return new PsEnviron(name);
}

// 0x005aea68
void PsEnviron::Init() {
    g_pfnNewEnviron = NewEnviron;
    g_pDefaultEnviron = new PsEnviron(HxStr("[default environ]"));
    g_pDefaultEnviron->mInternal = 1;
    g_pDefaultCam->AddDraw(g_pDefaultEnviron, nullptr);

    g_pDefaultLight = g_pfnNewLight(HxStr("[default light]"));
    g_pDefaultLight->mInternal = 1;
    g_pDefaultEnviron->AddLight(g_pDefaultLight);
    g_pDefaultCam->AddTrans(g_pDefaultLight);
}

// 0x005aecb8
int PsEnviron::DrawSelf() {
    g_nFogEnabled = mFogMode != kFogModeNone;
    if (g_nFogEnabled != 0) {
        g_flFogScale = kFogByteRange / (mFogStart - mFogEnd);
        g_flFogOffset = -mFogEnd * g_flFogScale;
    } else {
        g_flFogScale = 0.0f;
        g_flFogOffset = kFogByteRange;
    }

    const unsigned long long qwRed = static_cast<int>(mFogColor.r * kFogByteRange);
    const unsigned long long qwGreen = static_cast<int>(mFogColor.g * kFogByteRange);
    const unsigned long long qwBlue = static_cast<int>(mFogColor.b * kFogByteRange);
    g_gfxDevice.SetGsReg(kGsRegFogCol,
                         qwRed | (qwGreen << kFogColGreenShift) | (qwBlue << kFogColBlueShift),
                         kFogColMask);

    // The binary resizes both vectors from a temporary record whose padding words are 1.0.
    g_directionalLightRecords.resize(0);
    g_pointLightRecords.resize(0);

    for (Light *pLight : mLights) {
        if (pLight->mType == kLightTypeDirectional) {
            g_directionalLightRecords.resize(g_directionalLightRecords.size() + 1);
            DirectionalLightRecord &record = g_directionalLightRecords.back();
            Vector3 direction;
            direction.w = 1.0f;
            NegateVec3(pLight->mWorldXfm[kXfmRowLightAxis], &direction.x);
            record.mDirection = direction;
            record.mAmbient = pLight->mAmbient;
            record.mDiffuse = pLight->mDiffuse;
        } else if (pLight->mType == kLightTypePoint) {
            g_pointLightRecords.resize(g_pointLightRecords.size() + 1);
            PointLightRecord &record = g_pointLightRecords.back();
            const float *pTranslation = pLight->mWorldXfm[kXfmRowTranslation];
            record.mPosition.x = pTranslation[0];
            record.mPosition.y = pTranslation[1];
            record.mPosition.z = pTranslation[2];
            record.mPosition.w = pLight->mRange;
            record.mDiffuse = pLight->mDiffuse;
            record.mAmbient = pLight->mAmbient;
        }
    }

    g_pCurrentEnviron = this;
    return 1;
}

} // namespace Rnd
