#include "rnd/pscam.h"

#include <math.h>
#include <string.h>

#include "gfx/gfxdevice.h"
#include "gfx/renderstats.h"
#include "math/frustum.h"
#include "math/plane.h"
#include "math/transform.h"
#include "math/transformops.h"
#include "math/vector2.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/drawverts.h"
#include "rnd/mat.h"
#include "rnd/meshvert.h"
#include "rnd/particle.h"
#include "rnd/particlesys.h"
#include "rnd/psenviron.h"
#include "rnd/psmat.h"
#include "rnd/psmesh.h"
#include "rnd/pstex.h"
#include "rnd/tex.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

// Centre of the GS primitive coordinate space, as a float for the guard band and as an integer for
// the scissor conversion, and the four fractional bits of a primitive coordinate.
constexpr float kGsCoordinateCentre = 2048.0f;
constexpr int kGsCoordinateCentreInt = 0x800;
constexpr int kGsSubpixelShift = 4;

// Fraction of the screen rectangle each guard band axis is measured against, a 2 per cent margin
// over half the extent.
constexpr float kGuardBandHalfExtent = 0.51f;

// Depth range scaling and the bias the third viewport transform adds.
constexpr float kDepthRangeScale = 0.95f;
constexpr float kDepthBias = 0.0005f;
constexpr int kBitsPerByte = 8;

// SCISSOR_1 and the positions of its four fields.
constexpr int kGsRegScissor1 = 0x40;
constexpr unsigned long long kGsRegAllBits = ~0ULL;
constexpr int kScissorX1Shift = 16;
constexpr int kScissorY0Shift = 32;
constexpr int kScissorY1Shift = 48;

// Index of the first side plane in a frustum, after the front and back planes.
constexpr int kFirstSidePlane = 2;

// Store one plane into the flat plane array the draw path tests against.
inline void StoreDrawFrustumPlane(int nPlane, const Plane &plane) {
    memcpy(&g_afDrawFrustumPlanes[nPlane * kFrustumPlaneFloatCount], &plane, sizeof(plane));
}

// Read one plane back out of the same array.
inline Plane LoadDrawFrustumPlane(int nPlane) {
    Plane plane;
    memcpy(&plane, &g_afDrawFrustumPlanes[nPlane * kFrustumPlaneFloatCount], sizeof(plane));
    return plane;
}

// Widen one side plane of the local frustum by a guard band factor applied to the y component of
// its normal, retaining the point of the plane nearest the origin.
inline Plane WidenSidePlane(const Plane &plane, float flScale) {
    const float flDistance = -plane.d;
    Vector3 point;
    point.x = plane.a * flDistance;
    point.y = plane.b * flDistance;
    point.z = plane.c * flDistance;

    Vector3 normal;
    normal.x = plane.a;
    normal.y = plane.b * flScale;
    normal.z = plane.c;
    normal.w = 1.0f;
    Vec3Normalize(&normal.x, &normal.x);

    Plane widened;
    widened.a = normal.x;
    widened.b = normal.y;
    widened.c = normal.z;
    widened.d = -(normal.x * point.x + normal.y * point.y + normal.z * point.z);
    return widened;
}

// Clamp a coordinate of the screen rectangle to the unit range.
inline float ClampToUnit(float flValue) {
    if (flValue > 1.0f) {
        return 1.0f;
    }
    if (flValue < 0.0f) {
        return 0.0f;
    }
    return flValue;
}

// Vertical ratio of a four by three display, which is what a camera with no render target
// projects into.
constexpr float kDisplayYRatio = 0.75f;

// Where Init() places the default camera, 150 units back along the viewing axis.
constexpr float kDefaultCamDistance = -150.0f;

// Row of a transform that stores the translation.
constexpr int kXfmRowTranslation = 3;

// The VIF code that opens each VU1 setup block. UNPACK V4-32 to VU address 0, relative to TOPS.
constexpr unsigned kVifCmdUnpackV4_32 = 0x6c;
constexpr int kVifCmdShift = 24;
constexpr int kVifNumShift = 16;
constexpr unsigned kVifUnpackFlg = 0x8000;
constexpr int kEdgeSetupQuadwords = 8;
constexpr int kParticleSetupQuadwords = 9;

// GIFtag fields the setup blocks build. PRE enables the PRIM field, which starts at bit 47.
constexpr unsigned long long kGifTagEop = 1ULL << 15;
constexpr unsigned long long kGifTagPre = 1ULL << 46;
constexpr int kGifTagPrimShift = 47;
constexpr int kGifTagNRegShift = 60;

// PRIM fields. The edge block draws antialiased lines, and the particle block Gouraud-shaded
// sprites.
constexpr unsigned long long kPrimLineAntialiased = 0x81;
constexpr unsigned long long kPrimSpriteGouraud = 0xe;
constexpr int kPrimTmeShift = 4;
constexpr int kPrimFgeShift = 5;
constexpr int kPrimAbeShift = 6;

// Register lists. RGBAQ is 1, ST 2, XYZF2 4, and NOP 15.
constexpr unsigned long long kEdgeTagRegs = 0x441;
constexpr unsigned long long kEdgeTagNReg = 3;
constexpr unsigned long long kParticleTagRegsTextured = 0x412412;
constexpr unsigned long long kParticleTagRegsUntextured = 0x41f41f;
constexpr unsigned long long kParticleTagNReg = 6;

// Colour scales the setup blocks pass to the microprogram, and the byte scales of the edge colour.
// A texture bound without doubling takes the half scale.
constexpr float kColorScaleFull = 254.0f;
constexpr float kColorScaleHalf = 127.0f;
constexpr float kAlphaScale = 128.0f;
constexpr float kColorByteScale = 255.0f;

inline GifQuadword *TakeQuadword() {
    GifQuadword *pQuad = g_gfxDevice.mpWrite;
    g_gfxDevice.mpWrite = pQuad + 1;
    return pQuad;
}

// The packet buffer is untyped quadwords, so each typed payload is copied in by value.
inline void PushQuadword(const void *pPayload) {
    memcpy(TakeQuadword(), pPayload, sizeof(GifQuadword));
}

inline void PushFloats(float fl0, float fl1, float fl2, float fl3) {
    const float aflQuad[] = {fl0, fl1, fl2, fl3};
    PushQuadword(aflQuad);
}

inline void PushWords(unsigned n0, unsigned n1, unsigned n2, unsigned n3) {
    const unsigned anQuad[] = {n0, n1, n2, n3};
    PushQuadword(anQuad);
}

inline void PushVifUnpack(int nQuadwords) {
    PushWords(0,
              0,
              0,
              (kVifCmdUnpackV4_32 << kVifCmdShift) |
                  (static_cast<unsigned>(nQuadwords) << kVifNumShift) | kVifUnpackFlg);
}

inline void PushTransform(const Transform &xfm) {
    PushQuadword(&xfm.mBasisX);
    PushQuadword(&xfm.mBasisY);
    PushQuadword(&xfm.mBasisZ);
    PushQuadword(&xfm.mTranslation);
}

inline void
PushGifTag(unsigned long long qwPrim, unsigned long long qwNReg, unsigned long long qwRegs) {
    GifQuadword *pTag = TakeQuadword();
    pTag->mLo =
        (qwPrim << kGifTagPrimShift) | kGifTagPre | kGifTagEop | (qwNReg << kGifTagNRegShift);
    pTag->mHi = qwRegs;
}

// Texture coordinate generation modes TransformMeshVertsNoLight() handles. Explicit takes the
// vertex coordinates, and sphere mapping reflects the view axis off the normal.
constexpr int kGenModeExplicit = 0;
constexpr int kGenModeSphere = 1;
constexpr int kSphereMapRows = 3;
constexpr int kVectorComponents = 3;
constexpr int kComponentW = 3;

// The camera views along the second row of its world transform.
constexpr int kCamViewAxisRow = 1;

constexpr int kComponentX = 0;
constexpr int kComponentY = 1;
constexpr int kComponentZ = 2;
constexpr float kHalf = 0.5f;

// Colour scales PackParticleQuads() applies before converting to integers, and the alpha a line's
// trailing end point is drawn at.
constexpr Color kParticleColorScaleFull = {255.0f, 255.0f, 255.0f, 128.0f};
constexpr Color kParticleColorScaleHalf = {128.0f, 128.0f, 128.0f, 128.0f};
constexpr float kLineTailAlpha = 0.1f;
constexpr int kVertsPerSprite = 2;

// A GS coordinate carries four fractional bits.
constexpr float kFixed4Scale = 16.0f;

// Whether a clip-space position falls outside any of the six planes, as the vclipw flags report.
inline bool IsOutsideClipVolume(const float aflClipPos[kXfmRowFloatCount]) {
    const float flW = fabsf(aflClipPos[kComponentW]);
    for (int j = 0; j < kVectorComponents; ++j) {
        if (aflClipPos[j] > flW || aflClipPos[j] < -flW) {
            return true;
        }
    }
    return false;
}

// Write one packed vertex, the position converted to four fractional bits.
inline void StoreDrawVert(DrawVert &vert,
                          const float aflStq[kXfmRowFloatCount],
                          const unsigned anColor[kXfmRowFloatCount],
                          const float aflPosition[kXfmRowFloatCount]) {
    // The first quadword is S, T, Q, and the word the clipper reuses for its flags.
    memcpy(&vert, aflStq, sizeof(float) * kXfmRowFloatCount);
    memcpy(&vert.mColor, anColor, sizeof(vert.mColor));
    int anPosition[kXfmRowFloatCount];
    for (int j = 0; j < kXfmRowFloatCount; ++j) {
        anPosition[j] = static_cast<int>(aflPosition[j] * kFixed4Scale);
    }
    memcpy(&vert.mPos, anPosition, sizeof(vert.mPos));
}

// The face block. Triangles and fans take Gouraud shading unless the material is flat, and a
// textured pass sends ST ahead of RGBAQ and XYZF2 for every vertex.
constexpr int kFaceSetupQuadwords = 18;
constexpr unsigned long long kPrimTriangle = 3;
constexpr unsigned long long kPrimTriangleFan = 5;
constexpr int kPrimIipShift = 3;
constexpr unsigned long long kFaceTagNRegTextured = 9;
constexpr unsigned long long kFaceTagNRegUntextured = 6;
constexpr unsigned long long kFaceTagRegsTextured = 0x412412412ULL;
constexpr unsigned long long kFaceTagRegsUntextured = 0x414141;
constexpr unsigned long long kFanTagNReg = 3;
constexpr unsigned long long kFanTagRegsTextured = 0x412;
constexpr unsigned long long kFanTagRegsUntextured = 0x41f;
constexpr unsigned long long kMultiMeshTagRegsUntextured = 0x41f41f41fULL;
constexpr int kLightBlockQuadwords = 5;
constexpr int kMultiMeshLightBlockQuadwords = 4;

// The multi-mesh upload. Sixteen setup quadwords precede the vertices, four quadwords each, and
// the packed triangle indices follow as UNPACK V3-16, eight halfwords to a quadword.
constexpr int kMultiMeshSetupQuadwords = 16;
constexpr int kVu1VertQuadwords = 4;
constexpr int kFaceIndices = 3;
constexpr unsigned kVifCmdUnpackV3_16 = 0x69;
constexpr int kIndexHalfwordsPerQuadword = 8;
constexpr int kIndexQuadwordShift = 3;

// VU1 microprogram entries the face setup selects without a light.
constexpr int kVu1EntryUnlit = 0x2ee;
constexpr int kVu1EntryMultiMeshLit = 0x3d4;

// PRIM shading bits every face tag shares. The multi-mesh upload leaves fog out.
inline unsigned long long FaceShadingBits(bool bWithFog) {
    unsigned long long qwBits =
        (static_cast<unsigned long long>(g_nStageTextureBound) << kPrimTmeShift) |
        (static_cast<unsigned long long>(g_nSelectedFlat ^ 1) << kPrimIipShift) |
        (static_cast<unsigned long long>(g_nAlphaBlendEnabled) << kPrimAbeShift);
    if (bWithFog) {
        qwBits |= static_cast<unsigned long long>(g_nFogEnabled) << kPrimFgeShift;
    }
    return qwBits;
}

// The selected texture coordinate transform's first two rows and translation, or the identity
// with no translation when none is selected.
inline void PushUvXfm() {
    const Transform *pUvXfm = g_pSelectedUvXfm;
    if (pUvXfm != nullptr) {
        PushQuadword(&pUvXfm->mBasisX);
        PushQuadword(&pUvXfm->mBasisY);
        PushQuadword(&pUvXfm->mTranslation);
    } else {
        PushFloats(1.0f, 0.0f, 0.0f, 0.0f);
        PushFloats(0.0f, 1.0f, 0.0f, 0.0f);
        PushFloats(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

// The material's vertex colour flags, emissive first.
inline void PushMaterialVertexFlags() {
    const Mat *pMat = g_pSelectedMat;
    PushWords(static_cast<unsigned>(pMat->mVertEmissive),
              static_cast<unsigned>(pMat->mVertAmbient),
              static_cast<unsigned>(pMat->mVertDiffuse),
              static_cast<unsigned>(pMat->mVertAlpha));
}

// The colour scale the face and particle blocks share. It stays at full strength unless a texture
// is bound without doubling.
inline void PushColorScale() {
    const float flScale = (g_nStageTextureBound == 0 || g_nStageBlendDoubles != 0) ?
                              kColorScaleFull :
                              kColorScaleHalf;
    PushFloats(flScale, flScale, flScale, kAlphaScale);
}

} // namespace

// 0x00583358
int EmitFaceVu1Setup(const float *pXfm, const Sphere &sphere) {
    PushVifUnpack(kFaceSetupQuadwords);
    PushQuadword(&g_invGuardBandScale);

    Transform clipXfm;
    clipXfm.mTranslation.w = 1.0f;
    clipXfm.mBasisX.w = 1.0f;
    clipXfm.mBasisY.w = 1.0f;
    clipXfm.mBasisZ.w = 1.0f;
    Mat44Concat(&clipXfm.mBasisX.x, &g_viewProjectUnscaledXfm.mBasisX.x, pXfm);
    PushTransform(clipXfm);

    PushColorScale();
    PushFloats(g_viewportUnscaledXfm.mBasisX.x,
               g_viewportUnscaledXfm.mBasisY.y,
               g_viewportUnscaledXfm.mBasisZ.z,
               g_flFogScale);
    PushFloats(g_viewportUnscaledXfm.mTranslation.x,
               g_viewportUnscaledXfm.mTranslation.y,
               g_viewportUnscaledXfm.mTranslation.z,
               g_flFogOffset);
    PushUvXfm();

    const bool bTextured = g_nStageTextureBound != 0;
    const unsigned long long qwShading = FaceShadingBits(true);
    PushGifTag(kPrimTriangle | qwShading,
               bTextured ? kFaceTagNRegTextured : kFaceTagNRegUntextured,
               bTextured ? kFaceTagRegsTextured : kFaceTagRegsUntextured);
    PushGifTag(kPrimTriangleFan | qwShading,
               kFanTagNReg,
               bTextured ? kFanTagRegsTextured : kFanTagRegsUntextured);

    if (g_nLightingEnabled == 0) {
        g_gfxDevice.mpWrite += kLightBlockQuadwords;
        return kVu1EntryUnlit;
    }
    GifQuadword *pLight = TakeQuadword();
    PushQuadword(&g_pSelectedMat->mEmissive);
    GifQuadword *pAmbient = g_gfxDevice.mpWrite;
    PushQuadword(&g_pSelectedMat->mAmbient);
    GifQuadword *pDiffuse = g_gfxDevice.mpWrite;
    PushQuadword(&g_pSelectedMat->mDiffuse);
    const int nEntry = SelectLightForVertex(pLight, pAmbient, pDiffuse, pXfm, &sphere);
    PushMaterialVertexFlags();
    return nEntry;
}

// 0x00583ba0
int PsMesh::EmitMultiMeshFaceRun() {
    // The runs belong to whichever mesh owns the faces, a PsMesh on this target.
    const PsMesh *pOwner = static_cast<const PsMesh *>(mFacesOwner);
    const DrawRun &run = pOwner->mFaceRuns.front();
    const int nVertCount = static_cast<int>(run.mVertIndices.size());
    const int nIndexAddr = nVertCount * kVu1VertQuadwords + kMultiMeshSetupQuadwords;

    PushVifUnpack(nIndexAddr);
    PushTransform(g_viewProjectXfm);
    PushColorScale();
    PushFloats(g_viewportXfm.mBasisX.x, g_viewportXfm.mBasisY.y, g_viewportXfm.mBasisZ.z, 0.0f);
    PushFloats(g_viewportXfm.mTranslation.x,
               g_viewportXfm.mTranslation.y,
               g_viewportXfm.mTranslation.z,
               0.0f);
    PushUvXfm();
    PushGifTag(kPrimTriangle | FaceShadingBits(false),
               kFaceTagNRegTextured,
               g_nStageTextureBound != 0 ? kFaceTagRegsTextured : kMultiMeshTagRegsUntextured);

    int nEntry;
    if (g_nLightingEnabled != 0) {
        PushQuadword(&g_pSelectedMat->mEmissive);
        PushQuadword(&g_pSelectedMat->mAmbient);
        PushQuadword(&g_pSelectedMat->mDiffuse);
        PushMaterialVertexFlags();
        nEntry = kVu1EntryMultiMeshLit;
    } else {
        g_gfxDevice.mpWrite += kMultiMeshLightBlockQuadwords;
        nEntry = kVu1EntryUnlit;
    }

    const int nPrimCount = run.mIndexCount / kFaceIndices;
    PushWords(static_cast<unsigned>(nVertCount),
              static_cast<unsigned>(nPrimCount),
              static_cast<unsigned>(nEntry),
              static_cast<unsigned>(nVertCount * kVu1VertQuadwords + nPrimCount));

    const std::vector<MeshVert> &verts = mVertsOwner->mVerts;
    for (unsigned short nIndex : run.mVertIndices) {
        memcpy(g_gfxDevice.mpWrite, &verts[nIndex], sizeof(MeshVert));
        g_gfxDevice.mpWrite += kVu1VertQuadwords;
    }

    const int nIndexEnd = nIndexAddr + nPrimCount;
    PushWords(0,
              0,
              0,
              (kVifCmdUnpackV3_16 << kVifCmdShift) |
                  (static_cast<unsigned>(nPrimCount) << kVifNumShift) | kVifUnpackFlg |
                  static_cast<unsigned>(nIndexAddr));
    const int nIndexQuadwords =
        (run.mIndexCount + kIndexHalfwordsPerQuadword - 1) >> kIndexQuadwordShift;
    for (int i = 0; i < nIndexQuadwords; ++i) {
        memcpy(TakeQuadword(), &run.mIndices[i * kIndexHalfwordsPerQuadword], sizeof(GifQuadword));
    }
    return nIndexEnd;
}

// 0x005837d0
void EmitEdgeVu1Setup(const float *pXfm, const Color &color) {
    PushVifUnpack(kEdgeSetupQuadwords);

    // The concatenation never writes the fourth word of a row, so the binary presets all four.
    Transform clipXfm;
    clipXfm.mTranslation.w = 1.0f;
    clipXfm.mBasisX.w = 1.0f;
    clipXfm.mBasisY.w = 1.0f;
    clipXfm.mBasisZ.w = 1.0f;
    Mat44Concat(&clipXfm.mBasisX.x, &g_viewProjectXfm.mBasisX.x, pXfm);
    PushTransform(clipXfm);

    PushGifTag(kPrimLineAntialiased |
                   (static_cast<unsigned long long>(g_nFogEnabled) << kPrimFgeShift),
               kEdgeTagNReg,
               kEdgeTagRegs);
    PushFloats(g_viewportBiasedXfm.mBasisX.x,
               g_viewportBiasedXfm.mBasisY.y,
               g_viewportBiasedXfm.mBasisZ.z,
               g_flFogScale);
    PushFloats(g_viewportBiasedXfm.mTranslation.x,
               g_viewportBiasedXfm.mTranslation.y,
               g_viewportBiasedXfm.mTranslation.z,
               g_flFogOffset);
    PushWords(static_cast<unsigned>(color.r * kColorByteScale),
              static_cast<unsigned>(color.g * kColorByteScale),
              static_cast<unsigned>(color.b * kColorByteScale),
              static_cast<unsigned>(color.a * kAlphaScale));
}

// 0x00584980
int PackParticleQuads(DrawVert *pOutVerts, int nMode, const Particle *pFirst, int nLineLength) {
    if (pFirst == nullptr) {
        return 0;
    }
    const bool bFullColor = g_nStageTextureBound == 0 || g_nStageBlendDoubles != 0;

    // The binary concatenates the projection with an identity whose rows carry a fourth word of
    // 1.0.
    const float aflIdentity[kXfmRowCount][kXfmRowFloatCount] = {
        {1.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    };
    float aflClip[kXfmRowCount][kXfmRowFloatCount];
    Mat44Concat(&aflClip[0][0], &g_viewProjectXfm.mBasisX.x, &aflIdentity[0][0]);
    const Color &colorScale = bFullColor ? kParticleColorScaleFull : kParticleColorScaleHalf;
    const float aflViewScale[] = {
        g_viewportXfm.mBasisX.x, g_viewportXfm.mBasisY.y, g_viewportXfm.mBasisZ.z, 0.0f};
    const float aflViewOffset[] = {g_viewportXfm.mTranslation.x,
                                   g_viewportXfm.mTranslation.y,
                                   g_viewportXfm.mTranslation.z,
                                   0.0f};

    // Two vector registers are only ever partly written, so the lanes the routine does not write
    // carry over from particle to particle. Their first values are whatever the registers held.
    float aflStq[kXfmRowFloatCount] = {};
    float aflHalfExtent[kXfmRowFloatCount] = {};

    DrawVert *pOut = pOutVerts;
    const Particle *pParticle = pFirst;
    int nHistory = 0;
    while (pParticle != nullptr) {
        // A line's second end point is the history quadword nLineLength past the position, drawn
        // at a tenth of the alpha.
        Color color = pParticle->mCol;
        const Vector3 *pPosition = &pParticle->mPos;
        if (nHistory != 0) {
            color.a *= kLineTailAlpha;
            pPosition = &(&pParticle->mPos)[nHistory];
        }

        // On VU0 as vmulax, vmadday, vmaddaz, vmaddw, then vclipw against the w component.
        float aflClipPos[kXfmRowFloatCount];
        for (int j = 0; j < kXfmRowFloatCount; ++j) {
            aflClipPos[j] = aflClip[0][j] * pPosition->x + aflClip[1][j] * pPosition->y +
                            aflClip[2][j] * pPosition->z + aflClip[kXfmRowTranslation][j];
        }
        const unsigned anColor[] = {
            static_cast<unsigned>(static_cast<int>(colorScale.r * color.r)),
            static_cast<unsigned>(static_cast<int>(colorScale.g * color.g)),
            static_cast<unsigned>(static_cast<int>(colorScale.b * color.b)),
            static_cast<unsigned>(static_cast<int>(colorScale.a * color.a))};
        const float flQ = 1.0f / aflClipPos[kComponentW];

        if (IsOutsideClipVolume(aflClipPos)) {
            // A line whose second end point is outside withdraws the first as well.
            if (nMode == ParticleSys::kModeLine && nHistory != 0) {
                --pOut;
            }
            pParticle = pParticle->mNext;
            nHistory = 0;
            continue;
        }

        aflStq[kComponentZ] = 1.0f;
        float aflScreen[kXfmRowFloatCount];
        for (int j = 0; j < kVectorComponents; ++j) {
            aflStq[j] *= flQ;
            aflClipPos[j] *= flQ;
        }
        for (int j = 0; j < kXfmRowFloatCount; ++j) {
            aflScreen[j] = aflViewScale[j] * aflClipPos[j] + aflViewOffset[j];
        }

        if (nMode == ParticleSys::kModeSprite) {
            const float flHalfSize = pParticle->mSize * kHalf;
            aflHalfExtent[kComponentX] = g_particleScreenScale.x * flQ * flHalfSize;
            aflHalfExtent[kComponentY] = g_particleScreenScale.y * flQ * flHalfSize;
            float aflCorner[kXfmRowFloatCount];
            for (int j = 0; j < kXfmRowFloatCount; ++j) {
                aflCorner[j] = aflScreen[j] - aflHalfExtent[j];
            }
            const float aflNearStq[] = {0.0f, 0.0f, 0.0f, 1.0f};
            StoreDrawVert(pOut[0], aflNearStq, anColor, aflCorner);
            for (int j = 0; j < kXfmRowFloatCount; ++j) {
                aflCorner[j] = aflScreen[j] + aflHalfExtent[j];
            }
            aflStq[kComponentY] = aflStq[kComponentZ];
            aflStq[kComponentX] = aflStq[kComponentY];
            StoreDrawVert(pOut[1], aflStq, anColor, aflCorner);
            pOut += kVertsPerSprite;
        } else {
            StoreDrawVert(pOut[0], aflStq, anColor, aflScreen);
            ++pOut;
        }

        if (nMode == ParticleSys::kModeLine && nHistory == 0) {
            nHistory = nLineLength;
            continue;
        }
        pParticle = pParticle->mNext;
        nHistory = 0;
    }

    const int nVerts = static_cast<int>(pOut - pOutVerts);
    g_renderStats.mnVertsTransformed += nVerts;
    return nVerts;
}

// 0x00584700
void TransformMeshVertsNoLight(void *pOutVerts, MeshVert *pVerts, int nCount, const float *pXfm) {
    const Transform *pUvXfm = g_pSelectedUvXfm;
    const int nGenMode = g_nSelectedGenMode;
    if (pUvXfm == nullptr || nCount == 0) {
        return;
    }
    g_renderStats.mnVertsTransformed += nCount;

    // Sphere mapping reflects the view axis, the negated second row of the camera's world
    // transform, off each rotated normal. A stage transform is folded into both by its transpose.
    Vector3 view;
    float aflRotation[kSphereMapRows][kXfmRowFloatCount];
    if (nGenMode == kGenModeSphere) {
        Vector3 negated;
        negated.w = 1.0f;
        NegateVec3(g_pCurrentCam->mWorldXfm[kCamViewAxisRow], &negated.x);
        view = negated;
        memcpy(aflRotation, pXfm, sizeof(aflRotation));
        const Transform *pStageXfm = g_pSelectedStageXfm;
        if (pStageXfm != nullptr) {
            const float aflTransposed[kSphereMapRows][kXfmRowFloatCount] = {
                {pStageXfm->mBasisX.x, pStageXfm->mBasisY.x, pStageXfm->mBasisZ.x, 1.0f},
                {pStageXfm->mBasisX.y, pStageXfm->mBasisY.y, pStageXfm->mBasisZ.y, 1.0f},
                {pStageXfm->mBasisX.z, pStageXfm->mBasisY.z, pStageXfm->mBasisZ.z, 1.0f},
            };
            Vector3 stageView;
            stageView.w = 1.0f;
            TransformVec3ByMat3VU0(&view.x, &aflTransposed[0][0], &stageView.x);
            view = stageView;
            float aflStageRotation[kSphereMapRows][kXfmRowFloatCount];
            aflStageRotation[0][kComponentW] = 1.0f;
            aflStageRotation[1][kComponentW] = 1.0f;
            aflStageRotation[2][kComponentW] = 1.0f;
            MultiplyMat3VU0(&aflRotation[0][0], &aflTransposed[0][0], &aflStageRotation[0][0]);
            memcpy(aflRotation, aflStageRotation, sizeof(aflRotation));
        }
    }

    // A generation mode other than the two handled leaves the coordinates of the previous vertex,
    // which for the first vertex is whatever the vector register held.
    float flS = 0.0f;
    float flT = 0.0f;
    DrawVert *pOut = static_cast<DrawVert *>(pOutVerts);
    for (int i = 0; i < nCount; ++i) {
        const MeshVert &vert = pVerts[i];
        if (nGenMode == kGenModeExplicit) {
            flS = vert.mTex1.x;
            flT = vert.mTex1.y;
        } else if (nGenMode == kGenModeSphere) {
            // On VU0 as vmulax, vmadday, vmaddz for the rotation, then the reflection and a
            // reciprocal of the length through the Q register.
            float aflNormal[kVectorComponents];
            for (int j = 0; j < kVectorComponents; ++j) {
                aflNormal[j] = aflRotation[0][j] * vert.mNorm.x + aflRotation[1][j] * vert.mNorm.y +
                               aflRotation[2][j] * vert.mNorm.z;
            }
            const float flTwiceDot =
                2.0f * (view.x * aflNormal[0] + view.y * aflNormal[1] + view.z * aflNormal[2]);
            const float flReflectX = aflNormal[0] * flTwiceDot - view.x;
            const float flReflectY = aflNormal[1] * flTwiceDot - view.y - 1.0f;
            const float flReflectZ = aflNormal[2] * flTwiceDot - view.z;
            const float flInverseLength =
                1.0f /
                sqrtf(flReflectX * flReflectX + flReflectY * flReflectY + flReflectZ * flReflectZ);
            flS = flReflectX * flInverseLength;
            flT = flReflectZ * flInverseLength;
        }

        DrawVert &out = pOut[i];
        const float flU =
            pUvXfm->mBasisX.x * flS + pUvXfm->mBasisY.x * flT + pUvXfm->mTranslation.x;
        const float flV =
            pUvXfm->mBasisX.y * flS + pUvXfm->mBasisY.y * flT + pUvXfm->mTranslation.y;
        out.mS = flU * out.mQ;
        out.mT = flV * out.mQ;
    }
}

// 0x005839d0
void EmitParticleVu1Setup() {
    PushVifUnpack(kParticleSetupQuadwords);
    PushTransform(g_viewProjectXfm);
    PushColorScale();

    const unsigned long long qwPrim =
        (static_cast<unsigned long long>(g_nStageTextureBound) << kPrimTmeShift) |
        (static_cast<unsigned long long>(g_nAlphaBlendEnabled) << kPrimAbeShift) |
        kPrimSpriteGouraud;
    PushGifTag(qwPrim,
               kParticleTagNReg,
               g_nStageTextureBound != 0 ? kParticleTagRegsTextured : kParticleTagRegsUntextured);
    PushFloats(g_viewportXfm.mBasisX.x, g_viewportXfm.mBasisY.y, g_viewportXfm.mBasisZ.z, 0.0f);
    PushFloats(g_viewportXfm.mTranslation.x,
               g_viewportXfm.mTranslation.y,
               g_viewportXfm.mTranslation.z,
               0.0f);
    PushQuadword(&g_particleProjectScale);
}

// 0x00768410
PsCam *g_pDefaultCam;

// 0x00768420
float g_afDrawFrustumPlanes[kFrustumPlaneCount * kFrustumPlaneFloatCount];

// 0x008e4020
Transform g_viewProjectXfm;

// 0x008e4060
Transform g_viewProjectUnscaledXfm;

// 0x008e40a0
Transform g_viewportXfm;

// 0x008e40e0
Transform g_viewportUnscaledXfm;

// 0x008e4120
Transform g_viewportBiasedXfm;

// 0x008e4160
Vector3 g_particleScreenScale;

// 0x008e4170
Vector3 g_particleProjectScale;

// 0x008e4180
Vector3 g_guardBandScale;

// 0x008e4190
Vector3 g_invGuardBandScale;

// 0x008e41a0
float g_flCamNear;

// 0x008e41a4
int g_nScissorX0;

// 0x008e41a8
int g_nScissorX1;

// 0x008e41ac
int g_nScissorY0;

// 0x008e41b0
int g_nScissorY1;

// 0x00582558
PsCam::PsCam(const HxStr &name) : Object(name), Cam(name) {
}

// 0x005826e0
PsCam::~PsCam() {
}

// 0x00582830
int PsCam::DrawSelf() {
    int nTargetWidth;
    int nTargetHeight;
    if (mpTargetTex != nullptr) {
        static_cast<PsTex *>(mpTargetTex)->BindAsRenderTarget();
        nTargetWidth = mpTargetTex->mWidth;
        nTargetHeight = mpTargetTex->mHeight;
    } else {
        if (g_pCurrentCam != nullptr && g_pCurrentCam->mpTargetTex != nullptr) {
            g_gfxDevice.RestoreFrameBufferTarget();
        }
        nTargetWidth = g_gfxDevice.mnDisplayWidth;
        nTargetHeight = g_gfxDevice.mnDisplayHeight;
    }

    Vector2 centre;
    centre.x = mScreenRect.x + mScreenRect.w * 0.5f;
    centre.y = mScreenRect.y + mScreenRect.h * 0.5f;
    Vector2 half;
    half.x = 0.5f;
    half.y = 0.5f;
    Vector2 offset;
    SubVec2(&centre.x, &half.x, &offset.x);
    g_flCamNear = mNearPlane;

    const float flWidth = static_cast<float>(nTargetWidth);
    const float flHeight = static_cast<float>(nTargetHeight);
    g_guardBandScale.z = 1.0f;
    g_guardBandScale.x = (kGsCoordinateCentre - fabsf(offset.x * flWidth)) /
                         (mScreenRect.w * flWidth * kGuardBandHalfExtent);
    g_guardBandScale.y = (kGsCoordinateCentre - fabsf(offset.y * flHeight)) /
                         (mScreenRect.h * flHeight * kGuardBandHalfExtent);
    if (g_guardBandScale.x != 0.0f && g_guardBandScale.y != 0.0f) {
        g_invGuardBandScale.z = 1.0f;
        g_invGuardBandScale.x = 1.0f / g_guardBandScale.x;
        g_invGuardBandScale.y = 1.0f / g_guardBandScale.y;
    }

    StoreDrawFrustumPlane(0, mLocalFrustum.mFront);
    StoreDrawFrustumPlane(1, mLocalFrustum.mBack);
    StoreDrawFrustumPlane(kFirstSidePlane, WidenSidePlane(mLocalFrustum.mLeft, g_guardBandScale.x));
    StoreDrawFrustumPlane(kFirstSidePlane + 1,
                          WidenSidePlane(mLocalFrustum.mRight, g_guardBandScale.x));
    StoreDrawFrustumPlane(kFirstSidePlane + 2,
                          WidenSidePlane(mLocalFrustum.mTop, g_guardBandScale.y));
    StoreDrawFrustumPlane(kFirstSidePlane + 3,
                          WidenSidePlane(mLocalFrustum.mBottom, g_guardBandScale.y));

    // Every plane is transformed before any is stored back.
    Plane aWorldPlanes[kFrustumPlaneCount];
    for (int nPlane = 0; nPlane < kFrustumPlaneCount; ++nPlane) {
        aWorldPlanes[nPlane] =
            TransformPlaneToWorld(LoadDrawFrustumPlane(nPlane), &mWorldXfm[0][0]);
    }
    for (int nPlane = 0; nPlane < kFrustumPlaneCount; ++nPlane) {
        StoreDrawFrustumPlane(nPlane, aWorldPlanes[nPlane]);
    }

    // The three basis rows are set to the identity without touching their padding words.
    g_viewportXfm.mBasisX.x = 1.0f;
    g_viewportXfm.mBasisX.y = 0.0f;
    g_viewportXfm.mBasisX.z = 0.0f;
    g_viewportXfm.mBasisY.x = 0.0f;
    g_viewportXfm.mBasisY.y = 1.0f;
    g_viewportXfm.mBasisY.z = 0.0f;
    g_viewportXfm.mBasisZ.x = 0.0f;
    g_viewportXfm.mBasisZ.y = 0.0f;
    g_viewportXfm.mBasisZ.z = 1.0f;
    g_viewportXfm.mTranslation.x = 0.0f;
    g_viewportXfm.mTranslation.y = 0.0f;
    g_viewportXfm.mTranslation.z = 0.0f;
    g_viewportXfm.mTranslation.w = 1.0f;

    const float flHalfWidth = flWidth * mScreenRect.w * 0.5f;
    const float flHalfHeight = flHeight * mScreenRect.h * 0.5f;
    const int nMaxDepth = (1 << (g_gfxDevice.mnDepthBytes * kBitsPerByte)) - 1;
    const float flHalfDepth = static_cast<float>(nMaxDepth) * 0.5f;
    g_viewportXfm.mBasisX.x = flHalfWidth;
    g_viewportXfm.mBasisY.y = flHalfHeight;
    g_viewportXfm.mTranslation.x = offset.x * flWidth + kGsCoordinateCentre;
    g_viewportXfm.mTranslation.y = offset.y * flHeight + kGsCoordinateCentre;
    g_viewportXfm.mBasisZ.z = flHalfDepth * (mZRange.x - mZRange.y) * kDepthRangeScale;
    g_viewportXfm.mTranslation.z = flHalfDepth * (2.0f - mZRange.x - mZRange.y);
    g_viewportUnscaledXfm = g_viewportXfm;

    g_viewportXfm.mBasisY.y = flHalfHeight * g_guardBandScale.y;
    g_viewportXfm.mBasisX.x = flHalfWidth * g_guardBandScale.x;
    g_viewportBiasedXfm = g_viewportXfm;
    g_viewportBiasedXfm.mTranslation.z += flHalfDepth * (mZRange.y - mZRange.x) * kDepthBias;

    g_viewProjectXfm.mBasisX = mLocalProject[0];
    g_viewProjectXfm.mBasisY = mLocalProject[1];
    g_viewProjectXfm.mBasisZ = mLocalProject[2];
    g_viewProjectXfm.mTranslation = mLocalProject[3];
    g_viewProjectXfm.mBasisZ.w = 0.0f;
    g_viewProjectXfm.mBasisY.w = 0.0f;
    g_viewProjectXfm.mBasisX.w = 0.0f;
    g_viewProjectXfm.mTranslation.w = 1.0f;
    if (mFov == 0.0f) {
        g_viewProjectXfm.mBasisY.z = 2.0f / (mFarPlane - mNearPlane);
        g_viewProjectXfm.mTranslation.z = 1.0f - mFarPlane * g_viewProjectXfm.mBasisY.z;
    } else {
        g_viewProjectXfm.mTranslation.w = 0.0f;
        g_viewProjectXfm.mBasisY.w = g_viewProjectXfm.mBasisY.z;
        g_viewProjectXfm.mBasisY.z = (mFarPlane + mNearPlane) / (mFarPlane - mNearPlane);
        g_viewProjectXfm.mTranslation.z = mFarPlane - mFarPlane * g_viewProjectXfm.mBasisY.z;
    }

    const float flProjectX = g_viewProjectXfm.mBasisX.x / g_guardBandScale.x;
    const float flProjectY = g_viewProjectXfm.mBasisZ.y / g_guardBandScale.y;
    g_viewProjectUnscaledXfm = g_viewProjectXfm;
    g_particleScreenScale.z = 0.0f;
    g_particleProjectScale.z = 0.0f;
    g_viewProjectXfm.mBasisZ.y = flProjectY;
    g_particleProjectScale.x = flProjectX;
    g_viewProjectXfm.mBasisX.x = flProjectX;
    g_particleProjectScale.y = -flProjectY;
    g_particleScreenScale.x = flProjectX * g_viewportXfm.mBasisX.x;
    g_particleScreenScale.y = -flProjectY * g_viewportXfm.mBasisY.y;
    g_particleScreenScale.w = 0.0f;
    g_particleProjectScale.w = 0.0f;
    Mat44Concat(&g_viewProjectXfm.mBasisX.x, &g_viewProjectXfm.mBasisX.x, &mWorldToCam[0].x);
    Mat44Concat(&g_viewProjectUnscaledXfm.mBasisX.x,
                &g_viewProjectUnscaledXfm.mBasisX.x,
                &mWorldToCam[0].x);

    const float flLeft = ClampToUnit(mScreenRect.x);
    const float flTop = ClampToUnit(mScreenRect.y);
    const float flRight = ClampToUnit(mScreenRect.x + mScreenRect.w);
    const float flBottom = ClampToUnit(mScreenRect.y + mScreenRect.h);
    g_nScissorX1 = static_cast<int>(flRight * flWidth) - 1;
    g_nScissorY1 = static_cast<int>(flBottom * flHeight) - 1;
    g_nScissorX0 = static_cast<int>(flLeft * flWidth);
    g_nScissorY0 = static_cast<int>(flTop * flHeight);
    g_gfxDevice.SetGsReg(kGsRegScissor1,
                         static_cast<unsigned long long>(g_nScissorX0) |
                             (static_cast<unsigned long long>(g_nScissorX1) << kScissorX1Shift) |
                             (static_cast<unsigned long long>(g_nScissorY0) << kScissorY0Shift) |
                             (static_cast<unsigned long long>(g_nScissorY1) << kScissorY1Shift),
                         kGsRegAllBits);

    g_pCurrentCam = this;
    const int nOriginY =
        kGsCoordinateCentreInt - static_cast<int>(g_gfxDevice.mnDisplayHeight * 0.5f);
    const int nOriginX =
        kGsCoordinateCentreInt - static_cast<int>(g_gfxDevice.mnDisplayWidth * 0.5f);
    g_nScissorY1 = (nOriginY + g_nScissorY1) << kGsSubpixelShift;
    g_nScissorY0 = (nOriginY + g_nScissorY0) << kGsSubpixelShift;
    g_nScissorX1 = (nOriginX + g_nScissorX1) << kGsSubpixelShift;
    g_nScissorX0 = (nOriginX + g_nScissorX0) << kGsSubpixelShift;
    return 1;
}

// 0x005885b0
Vector2 PsCam::ScreenToPixels(const Vector2 &ptScreen) {
    Vector2 ptPixels;
    ptPixels.x = mScreenRect.x + ptScreen.x * mScreenRect.w;
    ptPixels.y = mScreenRect.y + ptScreen.y * mScreenRect.h;
    if (mpTargetTex != nullptr) {
        ptPixels.x *= static_cast<float>(mpTargetTex->mWidth);
        ptPixels.y *= static_cast<float>(mpTargetTex->mHeight);
    } else {
        ptPixels.y *= static_cast<float>(g_gfxDevice.mnDisplayHeight);
        ptPixels.x *= static_cast<float>(g_gfxDevice.mnDisplayWidth);
    }
    return ptPixels;
}

// 0x00588578
void PsCam::UpdateTargetAspect() {
    if (mpTargetTex == nullptr) {
        mYRatio = kDisplayYRatio;
    }
    Cam::UpdateTargetAspect();
}

// 0x00588498
void PsCam::SetTargetTex(Tex *pTex) {
    Cam::SetTargetTex(pTex);
    if (pTex == nullptr) {
        mYRatio = kDisplayYRatio;
    }
}

// 0x00588500
Cam *PsCam::NewCam(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Cam" and the object is 0x330 bytes.
    return new PsCam(name);
}

// 0x00582430
void PsCam::Init() {
    g_pfnNewCam = NewCam;
    g_pDefaultCam = new PsCam(HxStr("[default cam]"));
    g_pDefaultCam->mInternal = 1;

    // The binary assembles the row in a temporary and stores it as one quadword.
    float *pTranslation = g_pDefaultCam->mLocalXfm[kXfmRowTranslation];
    pTranslation[0] = 0.0f;
    pTranslation[1] = kDefaultCamDistance;
    pTranslation[2] = 0.0f;
    pTranslation[3] = 1.0f;
    g_pDefaultCam->mDirty = 1;
}

} // namespace Rnd
