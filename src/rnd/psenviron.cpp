#include "rnd/psenviron.h"

#include <math.h>
#include <string.h>
#include <vector>

#include "gfx/gfxdevice.h"
#include "math/sphere.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/drawverts.h"
#include "rnd/environ.h"
#include "rnd/light.h"
#include "rnd/mat.h"
#include "rnd/pscam.h"
#include "rnd/psmat.h"
#include "rnd/transformable.h"

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

// VU1 microprogram entries SelectLightForVertex() chooses between.
constexpr int kVu1EntryUnlit = 0x2ee;
constexpr int kVu1EntryDirectional = 0x2f8;
constexpr int kVu1EntryPoint = 0x35c;
constexpr int kVu1EntryNoLight = 0x3d4;

// The inverse of a rigid transform with the fourth word of every row preset, as the binary builds
// it before XfmInvertRigid() writes only the other words.
inline void InvertWithUnitW(const float *pXfm, float aflInverse[kXfmRowCount][kXfmRowFloatCount]) {
    for (int nRow = 0; nRow < kXfmRowCount; ++nRow) {
        aflInverse[nRow][kXfmRowFloatCount - 1] = 1.0f;
    }
    XfmInvertRigid(&aflInverse[0][0], pXfm);
}

// On VU0 as vmulax, vmadday, vmaddz. The fourth word is carried over from the source.
inline Vector3 RotateByRows(const float aflRows[kXfmRowCount][kXfmRowFloatCount],
                            const Vector3 &vec) {
    Vector3 rotated = vec;
    rotated.x = aflRows[0][0] * vec.x + aflRows[1][0] * vec.y + aflRows[2][0] * vec.z;
    rotated.y = aflRows[0][1] * vec.x + aflRows[1][1] * vec.y + aflRows[2][1] * vec.z;
    rotated.z = aflRows[0][2] * vec.x + aflRows[1][2] * vec.y + aflRows[2][2] * vec.z;
    return rotated;
}

// On VU0 as vmulax, vmadday, vmaddaz, vmaddw. The fourth word is carried over from the source.
inline Vector3 TransformByRows(const float aflRows[kXfmRowCount][kXfmRowFloatCount],
                               const Vector3 &vec) {
    Vector3 transformed = RotateByRows(aflRows, vec);
    transformed.x += aflRows[kXfmRowTranslation][0];
    transformed.y += aflRows[kXfmRowTranslation][1];
    transformed.z += aflRows[kXfmRowTranslation][2];
    return transformed;
}

// Whether a point light, already in the sphere's space, reaches a bounding sphere. A sphere with no
// radius is always reached. The range is the fourth word of the transformed position, which the
// transform carries over from the range in mPosition.
inline bool LightReachesSphere(const PointLightRecord &light, const Sphere *pSphere) {
    if (pSphere == nullptr || pSphere->mRadius == 0.0f) {
        return true;
    }
    Vector3 offset;
    offset.w = 1.0f;
    Vec3Sub(&light.mTransformedPosition.x, &pSphere->mCenter.x, &offset.x);
    const float flDistance = sqrtf(offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
    return flDistance <= light.mTransformedPosition.w + pSphere->mRadius;
}

// Scale a packet colour by a light colour, or replace it when the material takes that term from
// the vertex colours. The fourth word is untouched.
inline void ApplyLightColor(GifQuadword *pQuad, const Color &light, int bFromVertex) {
    Color color;
    memcpy(&color, pQuad, sizeof(color));
    if (bFromVertex == 0) {
        color.r *= light.r;
        color.g *= light.g;
        color.b *= light.b;
    } else {
        color.r = light.r;
        color.g = light.g;
        color.b = light.b;
    }
    memcpy(pQuad, &color, sizeof(color));
}

} // namespace

// 0x005af0c8
int TransformLightRecords(DirectionalLightRecord *&pDirectionalBegin,
                          DirectionalLightRecord *&pDirectionalEnd,
                          PointLightRecord *&pPointBegin,
                          PointLightRecord *&pPointEnd,
                          const float *pXfm,
                          const Sphere *pSphere) {
    int nActive = 0;
    pDirectionalBegin = g_directionalLightRecords.data();
    pDirectionalEnd = g_directionalLightRecords.data() + g_directionalLightRecords.size();
    pPointBegin = g_pointLightRecords.data();
    pPointEnd = g_pointLightRecords.data() + g_pointLightRecords.size();

    float aflInverse[kXfmRowCount][kXfmRowFloatCount];
    InvertWithUnitW(pXfm, aflInverse);

    for (DirectionalLightRecord *pLight = pDirectionalBegin; pLight != pDirectionalEnd; ++pLight) {
        pLight->mTransformedDirection = RotateByRows(aflInverse, pLight->mDirection);
        ++nActive;
    }
    for (PointLightRecord *pLight = pPointBegin; pLight != pPointEnd; ++pLight) {
        pLight->mTransformedPosition = TransformByRows(aflInverse, pLight->mPosition);
        pLight->mCulled = LightReachesSphere(*pLight, pSphere) ? 0 : 1;
        nActive += pLight->mCulled ^ 1;
    }
    return nActive;
}

// 0x005af2c0
int SelectLightForVertex(GifQuadword *pLight,
                         GifQuadword *pAmbient,
                         GifQuadword *pDiffuse,
                         const float *pXfm,
                         const Sphere *pSphere) {
    if (g_nLightingEnabled == 0) {
        return kVu1EntryUnlit;
    }

    float aflInverse[kXfmRowCount][kXfmRowFloatCount];
    if (!g_directionalLightRecords.empty()) {
        const DirectionalLightRecord &light = g_directionalLightRecords.front();
        InvertWithUnitW(pXfm, aflInverse);
        const Vector3 direction = RotateByRows(aflInverse, light.mDirection);
        memcpy(pLight, &direction, sizeof(direction));
        ApplyLightColor(pAmbient, light.mAmbient, g_pSelectedMat->mVertAmbient);
        ApplyLightColor(pDiffuse, light.mDiffuse, g_pSelectedMat->mVertDiffuse);
        return kVu1EntryDirectional;
    }

    if (g_pointLightRecords.empty()) {
        return kVu1EntryNoLight;
    }
    InvertWithUnitW(pXfm, aflInverse);
    for (PointLightRecord &light : g_pointLightRecords) {
        light.mTransformedPosition = TransformByRows(aflInverse, light.mPosition);
        if (LightReachesSphere(light, pSphere)) {
            ApplyLightColor(pAmbient, light.mAmbient, g_pSelectedMat->mVertAmbient);
            ApplyLightColor(pDiffuse, light.mDiffuse, g_pSelectedMat->mVertDiffuse);
            memcpy(pLight, &light.mTransformedPosition, sizeof(light.mTransformedPosition));
            return kVu1EntryPoint;
        }
    }
    return kVu1EntryNoLight;
}

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

// 0x005b2888
void PsEnviron::Terminate() {
    delete g_pDefaultEnviron;
    delete g_pDefaultLight;
    RegisterEnvironClass();
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
