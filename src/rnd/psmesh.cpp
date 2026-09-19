#include "rnd/psmesh.h"

#include "gfx/gfxdevice.h"
#include "gfx/renderstats.h"
#include "math/color.h"
#include "math/sphere.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/drawverts.h"
#include "rnd/mesh.h"
#include "rnd/psmat.h"
#include "rnd/transformable.h"
#include "rnd/view.h"

namespace Rnd {

namespace {

// GS general-purpose register indices. The shipped build took these from the PlayStation 2 SDK
// headers, which this tree does not reproduce.
constexpr int kGsRegTest1 = 0x47;
constexpr int kGsRegZbuf1 = 0x4e;

// ZBUF_1.ZMSK, which suppresses the depth write when it is set.
constexpr int kZbufZMaskShift = 32;
constexpr unsigned long long kZbufZMaskField = 1ULL << kZbufZMaskShift;

// TEST_1.ZTST. The GS compares a larger depth as nearer, so the inverse of each mesh depth
// function is what reaches the register.
constexpr int kTestZTestShift = 17;
constexpr unsigned long long kTestZTestField = 3ULL << kTestZTestShift;
constexpr int kGsZTestNever = 0;
constexpr int kGsZTestAlways = 1;
constexpr int kGsZTestGEqual = 2;
constexpr int kGsZTestGreater = 3;

// Quadwords of GIF space each pass reserves before it programs any register.
constexpr int kGifReserveQuadwords = 32;

// Passes beyond the second reuse the depth state the second pass programmed.
constexpr int kDepthProgramPassLimit = 2;

// Vertices the transformed vertex buffer holds. The software path reports an overflow rather than
// clipping the count.
constexpr int kMaxSoftwareVerts = 2500;

// 0x00607268. Clamps all four components through VU0 macro mode, with the source and the
// destination aliased at its one call site.
void ClampColorToUnitRange(const Color &in, Color &out) {
    constexpr float kMin = 0.0f;
    constexpr float kMax = 1.0f;
    out.r = in.r < kMin ? kMin : (in.r > kMax ? kMax : in.r);
    out.g = in.g < kMin ? kMin : (in.g > kMax ? kMax : in.g);
    out.b = in.b < kMin ? kMin : (in.b > kMax ? kMax : in.b);
    out.a = in.a < kMin ? kMin : (in.a > kMax ? kMax : in.a);
}

// A negative cap draws every vertex. 0x006023f4, 0x00602448, and 0x0060258c all expand this.
inline int DrawVertCount(int nMaxVerts, int nVerts) {
    if (nMaxVerts < 0 || nVerts < nMaxVerts) {
        return nVerts;
    }
    return nMaxVerts;
}

// 0x006022e8 and 0x006024bc both expand this.
inline void SelectDepthRegs(int nZMask, int nZTest) {
    g_gfxDevice.SetGsReg(
        kGsRegZbuf1, static_cast<unsigned long long>(nZMask) << kZbufZMaskShift, kZbufZMaskField);
    g_gfxDevice.SetGsReg(
        kGsRegTest1, static_cast<unsigned long long>(nZTest) << kTestZTestShift, kTestZTestField);
}

} // namespace

// 0x00602600
PsMesh::PsMesh(const HxStr &name) : Mesh(name) {
}

// 0x00605f48
PsMesh::~PsMesh() {
}

// 0x00602128
int PsMesh::DrawSelf() {
    ++g_renderStats.mnMeshDraws;

    Sphere worldSphere;
    worldSphere.mCenter.w = 1.0f;
    if (PrepareDraw(worldSphere) == 0) {
        // A culled mesh that owns geometry suppresses its children as well, and one with neither
        // faces nor edges lets them draw.
        return mFacesOwner->mFaces.size() == 0 && mFacesOwner->mEdges.size() == 0;
    }

    const int nUseVu1 = g_gfxDevice.mnUseVu1;
    if (nUseVu1 == 0 && static_cast<int>(mVertsOwner->mVerts.size()) > kMaxSoftwareVerts) {
        g_failSink.Report("DrawShowing vert buffer overflow... %d\n", mVertsOwner->mVerts.size());
        if (g_failSink.mAbortProc != nullptr) {
            g_failSink.mAbortProc();
        }
        return 0;
    }

    int nClip = 1;
    if (mSphere.mRadius != 0.0f && IsSphereInsideFrustum(worldSphere, g_afDrawFrustumPlanes) != 0) {
        nClip = 0;
    }

    const float *pXfm = mTransOwner->GetDrawXfm();
    int nPass = 0;
    int nMorePasses = 0;
    if (mFacesOwner->mFaces.size() != 0) {
        do {
            g_gfxDevice.ReserveGifSpace(kGifReserveQuadwords);
            if (g_pCurrentCam->mnSuppressDepthRegs == 0 && nPass < kDepthProgramPassLimit) {
                int nZMask = 0;
                if (nPass != 0 || mZMode == kZModeDisable || mZMode == kZModeZReadOnly ||
                    mZMode == kZModeWReadOnly) {
                    nZMask = 1;
                }

                int nZTest = kGsZTestGEqual;
                if (mZMode == kZModeDisable || mZFunc == kZFuncAlways) {
                    nZTest = kGsZTestAlways;
                } else if (nPass == 0 && mZFunc != kZFuncLessEqual && mZFunc != kZFuncEqual) {
                    nZTest = mZFunc == kZFuncLess ? kGsZTestGreater : kGsZTestNever;
                }
                SelectDepthRegs(nZMask, nZTest);
            }

            if (mMat != nullptr) {
                // The narrowing is what psmat.h records as undecidable. Every material on this
                // target is a PsMat, because GfxDevice::Init() installs that creator.
                nMorePasses = static_cast<PsMat *>(mMat)->Select();
            } else {
                PsMat::SelectDefault();
            }

            if (nUseVu1 != 0) {
                DrawFacesVU1(pXfm, nClip);
            } else {
                const int nVerts =
                    DrawVertCount(mMaxVerts, static_cast<int>(mVertsOwner->mVerts.size()));
                if (nPass == 0) {
                    TransformAndLightMeshVerts(
                        g_aDrawVerts, pXfm, &mVertsOwner->mVerts[0], nVerts, nClip, mSphere);
                } else {
                    TransformMeshVertsNoLight(g_aDrawVerts, &mVertsOwner->mVerts[0], nVerts, pXfm);
                }
                DrawFacesSoftware(nClip);
            }
            ++nPass;
        } while (nMorePasses != 0);
    }

    if (mFacesOwner->mEdges.size() == 0) {
        return 1;
    }

    g_gfxDevice.ReserveGifSpace(kGifReserveQuadwords);
    if (g_pCurrentCam->mnSuppressDepthRegs == 0) {
        int nZTest = kGsZTestGEqual;
        if (mZMode == kZModeDisable || mZFunc == kZFuncAlways) {
            nZTest = kGsZTestAlways;
        }
        SelectDepthRegs(1, nZTest);
    }

    if (mMat != nullptr) {
        static_cast<PsMat *>(mMat)->SelectAlphaBlend();
    } else if (mFacesOwner->mFaces.size() == 0) {
        PsMat::SelectDefault();
    }

    if (nUseVu1 != 0) {
        DrawEdgesVU1(pXfm);
        return 1;
    }

    // The face pass has already filled the buffer unless there were no faces to draw.
    if (mFacesOwner->mFaces.size() == 0) {
        const int nVerts = DrawVertCount(mMaxVerts, static_cast<int>(mVertsOwner->mVerts.size()));
        TransformAndLightMeshVerts(
            g_aDrawVerts, pXfm, &mVertsOwner->mVerts[0], nVerts, nClip, mSphere);
    }
    DrawEdgesSoftware(nClip);
    return 1;
}

// 0x00606a00
void PsMesh::Refresh() {
    Mesh::Refresh();
    for (auto &vert : mVerts) {
        ClampColorToUnitRange(vert.mColor, vert.mColor);
    }
}

// 0x00606928
Mesh *NewPsMesh(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Mesh" and rounds the 0x170-byte object up
    // to 0x180 bytes.
    return new PsMesh(name);
}

} // namespace Rnd
