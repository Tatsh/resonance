#include "rnd/psmesh.h"

#include <algorithm>

#include "gfx/gfxdevice.h"
#include "gfx/renderstats.h"
#include "math/color.h"
#include "math/sphere.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/drawverts.h"
#include "rnd/mat.h"
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

// GS PRIM fields. Both software paths program the whole of the register that matters and leave
// the context and fixed-fragment bits alone.
constexpr int kGsRegPrim = 0;
constexpr unsigned kGsPrimLine = 1;
constexpr unsigned kGsPrimTriangle = 3;
constexpr int kGsPrimIipShift = 3;
constexpr int kGsPrimTmeShift = 4;
constexpr int kGsPrimFgeShift = 5;
constexpr int kGsPrimAbeShift = 6;
constexpr unsigned kGsPrimAa1 = 1u << 7;
constexpr unsigned long long kGsPrimFieldMask = 0x7ff;

// GIFtag register descriptors, four bits each, consumed from the low nibble upward. Every packet
// here is the PACKED format, so FLG stays zero and one register spends one quadword.
constexpr unsigned long long kGifRegsTexturedTri = 0x412412412ULL;
constexpr unsigned long long kGifRegsUntexturedTri = 0x414141ULL;
constexpr unsigned long long kGifRegsLine = 0x441ULL;
constexpr int kGifNRegTexturedTri = 9;
constexpr int kGifNRegUntexturedTri = 6;
constexpr int kGifNRegLine = 3;
constexpr int kGifTagNRegShift = 60;

// Quadwords of a transformed vertex each path submits. A textured triangle sends the texture
// coordinate, the colour, and the position; an untextured one starts at the colour; a line takes
// its colour from the material and sends the position alone.
constexpr int kDrawVertQuadwordsTextured = 3;
constexpr int kDrawVertQuadwordsUntextured = 2;
constexpr int kDrawVertQuadwordsLine = 1;

// The GS takes eight-bit colour and seven-bit alpha, so alpha scales to half of the full range.
constexpr float kGsColorScale = 255.0f;
constexpr float kGsAlphaScale = 128.0f;

// VU1 destination quadwords a run may occupy, and what each item spends. A face vertex lands as
// four quadwords and an edge vertex as two, which is the destination stride the write cycle of
// each VU1 batch arranges, and each primitive spends one more for its indices. A run closes
// rather than starting a primitive it cannot finish, so the reserve covers a whole one.
constexpr int kFaceRunQuadwordBudget = 315;
constexpr int kEdgeRunQuadwordBudget = 325;
constexpr int kFaceVertQuadwordCost = 4;
constexpr int kEdgeVertQuadwordCost = 2;
constexpr int kFaceRunReserve = 5;
constexpr int kEdgeRunReserve = 3;

// Scratch capacity the strip builder starts each list at.
constexpr int kRunVertCapacity = 128;
constexpr int kRunIndexCapacity = 330;

// A run index addresses VU1 memory rather than the vertex list, so it is scaled by the quadwords
// a vertex occupies there.
constexpr int kVu1VertStride = 4;

// The clipper appends its output above the mesh vertex cap and reports the next free index. A
// result of fewer than three vertices describes nothing.
constexpr int kClipScratchBase = kDrawVertCapacity;
constexpr int kClipMinFanIndex = kDrawVertCapacity + 3;

// VIFcode CMD field values, from the codes the two VU1 paths assemble.
constexpr unsigned kVifCmdStCycl = 0x01;
constexpr unsigned kVifCmdMsCal = 0x14;
constexpr unsigned kVifCmdMsCnt = 0x17;
constexpr unsigned kVifCmdUnpackV2_16 = 0x65;
constexpr unsigned kVifCmdUnpackV3_16 = 0x69;
constexpr unsigned kVifCmdUnpackV4_32 = 0x6c;
constexpr int kVifCmdShift = 24;
constexpr int kVifNumShift = 16;

// Bit 15 of an UNPACK immediate, which adds the double-buffer offset to the destination address.
constexpr unsigned kVifUnpackFlg = 0x8000;

// STCYCL immediates, as the write length in the upper byte and the cycle length in the lower one.
constexpr unsigned kVifStCyclWl1Cl1 = 0x0101;
constexpr unsigned kVifStCyclWl1Cl2 = 0x0102;
constexpr unsigned kVifStCyclWl3Cl4 = 0x0304;

// VU1 data addresses each batch unpacks to. A face batch puts its parameter quadword at the first
// and its vertices immediately after; an edge batch separates the two.
constexpr int kVu1FaceParamAddr = 0x12;
constexpr int kVu1EdgeParamAddr = 8;
constexpr int kVu1EdgeVertAddr = 9;

// Microprogram entry points. A textured face batch enters at the start of the program and an
// untextured one skips to the shorter path.
constexpr int kVu1FaceEntryTextured = 0x000;
constexpr int kVu1FaceEntryUntextured = 0x4ce;
constexpr int kVu1EdgeEntry = 0x1c2;

// A VIFcode NUM field is eight bits, so one UNPACK cannot describe more destination quadwords
// than this. The textured face path closes and reopens its batch on reaching the limit.
constexpr int kVifUnpackQuadwordLimit = 0xfc;

// Quadwords of a Rnd::MeshVert each path sends. An untextured face batch omits the texture
// coordinate quadword and an edge batch sends the position alone, but the destination stride
// stays at four either way, which is what the write cycle of each batch arranges.
constexpr int kVertQuadwordsTextured = 4;
constexpr int kVertQuadwordsUntextured = 3;
constexpr int kVertQuadwordsEdge = 1;
constexpr int kVertQuadwordStride = 4;

// Indices one primitive consumes, and how many 16-bit indices fit in a quadword.
constexpr int kFaceIndicesPerPrim = 3;
constexpr int kEdgeIndicesPerPrim = 2;
constexpr int kIndexHalfwordsPerQuadword = 8;
constexpr int kIndexHalfwordShift = 3;

// Assemble one VIFcode.
inline unsigned MakeVifCode(unsigned nCmd, int nNum, unsigned nImmediate) {
    return (nCmd << kVifCmdShift) | (static_cast<unsigned>(nNum) << kVifNumShift) | nImmediate;
}

// Two 32-bit words ride in each half of a packet quadword, the first in the low bits.
inline unsigned long long PackWordPair(unsigned nLow, unsigned nHigh) {
    return nLow | (static_cast<unsigned long long>(nHigh) << 32);
}

// Take the next quadword of the packet buffer.
inline GifQuadword *TakeQuadword() {
    GifQuadword *pQuad = g_gfxDevice.mpWrite;
    g_gfxDevice.mpWrite = pQuad + 1;
    return pQuad;
}

// The packet buffer is untyped quadwords. These are the only two places a typed record is copied
// into it, a vertex and a packed index block, so the one conversion lives here.
inline const GifQuadword *AsQuadwords(const void *pRecord) {
    return static_cast<const GifQuadword *>(pRecord);
}

inline GifQuadword *AsWritableQuadwords(void *pRecord) {
    return static_cast<GifQuadword *>(pRecord);
}

// Copy a packed index block into the stream a quadword at a time. Both VU1 paths expand this.
inline void AppendIndexData(const unsigned short *pIndices, int nIndexCount) {
    const GifQuadword *pSource = AsQuadwords(pIndices);
    const int nQuadwords =
        (nIndexCount + kIndexHalfwordsPerQuadword - 1) / kIndexHalfwordsPerQuadword;
    for (int i = 0; i < nQuadwords; ++i) {
        GifQuadword *pDest = g_gfxDevice.mpWrite;
        *pDest = pSource[i];
        g_gfxDevice.mpWrite = pDest + 1;
    }
}

// Append part of a vertex and report where the next one goes. The caller writes the advanced
// pointer back to the device once, rather than once per vertex.
inline GifQuadword *AppendVert(GifQuadword *pWrite, const MeshVert &vert, int nQuadwords) {
    const GifQuadword *pSource = AsQuadwords(&vert);
    for (int i = 0; i < nQuadwords; ++i) {
        pWrite[i] = pSource[i];
    }
    return pWrite + nQuadwords;
}

// Append part of a transformed vertex to the stream, one quadword at a time as the binary does.
inline void AppendDrawVert(const DrawVert &vert, int nFirstQuadword, int nQuadwords) {
    const GifQuadword *pSource = AsQuadwords(&vert) + nFirstQuadword;
    for (int i = 0; i < nQuadwords; ++i) {
        GifQuadword *pDest = g_gfxDevice.mpWrite;
        *pDest = pSource[i];
        g_gfxDevice.mpWrite = pDest + 1;
    }
}

// 0x00606a60
// Submit one triangle of transformed vertices, clipping it first when asked.
//
// Rnd::PsMesh::DrawFacesSoftware() inlines one level of this and the recursion uses the body here.
// A triangle no plane crosses goes straight out. One that any vertex places beyond the far plane
// is dropped, as is one whose three vertices share a plane, because that puts the whole triangle
// outside it. Anything else is clipped into the scratch above the vertex cap and the result fans
// out as triangles, each submitted on its own so that a full buffer is sent mid-fan.
void EmitTriangle(unsigned nIdx0, unsigned nIdx1, unsigned nIdx2, int bClip) {
    if (bClip != 0) {
        const int nFlags = g_aDrawVerts[nIdx0].mClipFlags | g_aDrawVerts[nIdx1].mClipFlags |
                           g_aDrawVerts[nIdx2].mClipFlags;
        if ((nFlags & kDrawVertClipAnyPlane) != 0) {
            ++g_renderStats.mnFacesClipped;
            if ((nFlags & kDrawVertClipFarPlane) != 0) {
                return;
            }
            const int nShared = g_aDrawVerts[nIdx0].mClipFlags & g_aDrawVerts[nIdx1].mClipFlags &
                                g_aDrawVerts[nIdx2].mClipFlags;
            if ((nShared & kDrawVertClipOtherPlanes) != 0) {
                return;
            }

            int nNextIndex = kClipScratchBase;
            ClipTriangleToFrustum(nIdx0, nIdx1, nIdx2, g_aDrawVerts, &nNextIndex);
            while (nNextIndex >= kClipMinFanIndex) {
                const unsigned nFanIndex = static_cast<unsigned>(nNextIndex);
                EmitTriangle(
                    kClipScratchBase, (nFanIndex - 2) & 0xffff, (nFanIndex - 1) & 0xffff, 0);
                g_gfxDevice.FlushGifPacket(1, 1);
                --nNextIndex;
            }
            return;
        }
    }

    // The second and third vertices precede the first, which is the order the strips Sync() builds
    // expect.
    if (g_nStageTextureBound != 0) {
        AppendDrawVert(g_aDrawVerts[nIdx1], 0, kDrawVertQuadwordsTextured);
        AppendDrawVert(g_aDrawVerts[nIdx2], 0, kDrawVertQuadwordsTextured);
        AppendDrawVert(g_aDrawVerts[nIdx0], 0, kDrawVertQuadwordsTextured);
    } else {
        AppendDrawVert(g_aDrawVerts[nIdx1], 1, kDrawVertQuadwordsUntextured);
        AppendDrawVert(g_aDrawVerts[nIdx2], 1, kDrawVertQuadwordsUntextured);
        AppendDrawVert(g_aDrawVerts[nIdx0], 1, kDrawVertQuadwordsUntextured);
    }
    ++g_renderStats.mnTriangles;
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

// 0x00606d38
void PsMesh::SelectDepthRegsForPass(const Mesh &mesh, int nPass) {
    if (g_pCurrentCam->mpTargetTex != nullptr || nPass >= kDepthProgramPassLimit) {
        return;
    }

    int nZMask = 0;
    if (nPass != 0 || mesh.mZMode == kZModeDisable || mesh.mZMode == kZModeZReadOnly ||
        mesh.mZMode == kZModeWReadOnly) {
        nZMask = 1;
    }

    int nZTest = kGsZTestGEqual;
    if (mesh.mZMode == kZModeDisable || mesh.mZFunc == kZFuncAlways) {
        nZTest = kGsZTestAlways;
    } else if (nPass == 0 && mesh.mZFunc != kZFuncLessEqual && mesh.mZFunc != kZFuncEqual) {
        nZTest = mesh.mZFunc == kZFuncLess ? kGsZTestGreater : kGsZTestNever;
    }
    SelectDepthRegs(nZMask, nZTest);
}

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
        } else {
            throw; // With no handler the binary rethrows the exception in flight.
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
            if (g_pCurrentCam->mpTargetTex == nullptr && nPass < kDepthProgramPassLimit) {
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
    if (g_pCurrentCam->mpTargetTex == nullptr) {
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

// 0x006019e0
void PsMesh::DrawFacesVU1(const float *pXfm, int nClip) {
    g_renderStats.mnTriangles += static_cast<int>(mFacesOwner->mFaces.size());
    g_gfxDevice.CloseGifTag(1);
    g_gfxDevice.SwapGifWrite();
    const int nSetup = EmitFaceVu1Setup(pXfm, mSphere);
    const int nTextured = g_nStageTextureBound;

    // The batches belong to whichever mesh owns the faces, and on this target that mesh is a
    // PsMesh, because GfxDevice::Init() installs the PsMesh creator over the mesh hook.
    PsMesh *pOwner = static_cast<PsMesh *>(mFacesOwner);
    for (auto &run : pOwner->mFaceRuns) {
        const int nVertCount = static_cast<int>(run.mVertIndices.size());
        const int nPrimCount = run.mIndexCount / kFaceIndicesPerPrim;

        // The header quadword reaches the hardware before the batch length is known, so its
        // VIFcode slot is reserved here and filled in once the vertices have been counted.
        GifQuadword *pBatchCode = TakeQuadword();
        pBatchCode->mLo = 0;
        pBatchCode->mHi = 0;
        GifQuadword *pParam = TakeQuadword();
        pParam->mLo = PackWordPair(nVertCount, nPrimCount);
        pParam->mHi = PackWordPair(nSetup, nClip);

        GifQuadword *pWrite = g_gfxDevice.mpWrite;
        int nVuAddr = kVu1FaceParamAddr;
        int nBatchQuadwords = 1;
        if (nTextured != 0) {
            for (auto nIndex : run.mVertIndices) {
                if (nBatchQuadwords >= kVifUnpackQuadwordLimit) {
                    pBatchCode->mHi =
                        PackWordPair(0,
                                     MakeVifCode(kVifCmdUnpackV4_32,
                                                 nBatchQuadwords,
                                                 kVifUnpackFlg | static_cast<unsigned>(nVuAddr)));
                    nVuAddr += nBatchQuadwords;
                    nBatchQuadwords = 0;
                    pBatchCode = pWrite;
                    pBatchCode->mLo = 0;
                    pBatchCode->mHi = 0;
                    ++pWrite;
                }
                pWrite = AppendVert(pWrite, mVertsOwner->mVerts[nIndex], kVertQuadwordsTextured);
                nBatchQuadwords += kVertQuadwordsTextured;
            }
            pBatchCode->mHi =
                PackWordPair(0,
                             MakeVifCode(kVifCmdUnpackV4_32,
                                         nBatchQuadwords,
                                         kVifUnpackFlg | static_cast<unsigned>(nVuAddr)));
        } else {
            // Without a texture the parameter quadword travels under its own UNPACK, because the
            // vertices that follow it need a write cycle it must not be subject to.
            pBatchCode->mHi = PackWordPair(
                0, MakeVifCode(kVifCmdUnpackV4_32, 1, kVifUnpackFlg | kVu1FaceParamAddr));
            nVuAddr = kVu1FaceParamAddr + 1;
            nBatchQuadwords = nVertCount * kVertQuadwordStride;

            GifQuadword *pVertCode = pWrite;
            ++pWrite;
            pVertCode->mLo = 0;
            pVertCode->mHi =
                PackWordPair(MakeVifCode(kVifCmdStCycl, 0, kVifStCyclWl3Cl4),
                             MakeVifCode(kVifCmdUnpackV4_32,
                                         nVertCount * kVertQuadwordsUntextured,
                                         kVifUnpackFlg | static_cast<unsigned>(nVuAddr)));
            for (auto nIndex : run.mVertIndices) {
                pWrite = AppendVert(pWrite, mVertsOwner->mVerts[nIndex], kVertQuadwordsUntextured);
            }
        }
        nVuAddr += nBatchQuadwords;
        g_gfxDevice.mpWrite = pWrite;

        GifQuadword *pIndexCode = TakeQuadword();
        pIndexCode->mLo = 0;
        pIndexCode->mHi = PackWordPair(MakeVifCode(kVifCmdStCycl, 0, kVifStCyclWl1Cl1),
                                       MakeVifCode(kVifCmdUnpackV3_16,
                                                   nPrimCount,
                                                   kVifUnpackFlg | static_cast<unsigned>(nVuAddr)));
        AppendIndexData(run.mIndices, run.mIndexCount);

        g_gfxDevice.FlushReservedGif();
        GifQuadword *pEntry = TakeQuadword();
        pEntry->mHi = 0;
        if (&run == &pOwner->mFaceRuns.front()) {
            pEntry->mLo =
                MakeVifCode(kVifCmdMsCal,
                            0,
                            nTextured != 0 ? kVu1FaceEntryTextured :
                                             static_cast<unsigned>(kVu1FaceEntryUntextured));
        } else {
            pEntry->mLo = MakeVifCode(kVifCmdMsCnt, 0, 0);
        }
        g_gfxDevice.FlushGifPacket(0, 0);
    }
}

// 0x00601de0
void PsMesh::DrawEdgesVU1(const float *pXfm) {
    g_renderStats.mnLines += static_cast<int>(mFacesOwner->mEdges.size());
    g_gfxDevice.CloseGifTag(1);
    g_gfxDevice.SwapGifWrite();

    Color edgeColor;
    if (mMat != nullptr) {
        edgeColor = mMat->mSpecular;
    } else {
        edgeColor.r = 1.0f;
        edgeColor.g = 1.0f;
        edgeColor.b = 1.0f;
        edgeColor.a = 1.0f;
    }
    EmitEdgeVu1Setup(pXfm, edgeColor);

    PsMesh *pOwner = static_cast<PsMesh *>(mFacesOwner);
    for (auto &run : pOwner->mEdgeRuns) {
        // An edge batch knows its length up front, because it never splits, so the header
        // VIFcode is complete when it is written.
        GifQuadword *pParamCode = TakeQuadword();
        pParamCode->mLo = 0;
        pParamCode->mHi =
            PackWordPair(0, MakeVifCode(kVifCmdUnpackV4_32, 1, kVifUnpackFlg | kVu1EdgeParamAddr));

        const int nVertCount = static_cast<int>(run.mVertIndices.size());
        const int nPrimCount = run.mIndexCount / kEdgeIndicesPerPrim;
        GifQuadword *pParam = TakeQuadword();
        pParam->mLo = PackWordPair(nVertCount, nPrimCount);
        pParam->mHi = 0;

        GifQuadword *pVertCode = TakeQuadword();
        pVertCode->mLo = 0;
        pVertCode->mHi = PackWordPair(
            MakeVifCode(kVifCmdStCycl, 0, kVifStCyclWl1Cl2),
            MakeVifCode(kVifCmdUnpackV4_32, nVertCount, kVifUnpackFlg | kVu1EdgeVertAddr));

        GifQuadword *pWrite = g_gfxDevice.mpWrite;
        for (auto nIndex : run.mVertIndices) {
            pWrite = AppendVert(pWrite, mVertsOwner->mVerts[nIndex], kVertQuadwordsEdge);
        }
        g_gfxDevice.mpWrite = pWrite;

        GifQuadword *pIndexCode = TakeQuadword();
        pIndexCode->mLo = 0;
        // The immediate is shifted one short of the NUM field, which puts the low bit of the
        // halved index count on the FLG bit that the next term sets regardless.
        pIndexCode->mHi = PackWordPair(
            MakeVifCode(kVifCmdStCycl, 0, kVifStCyclWl1Cl1),
            (kVifCmdUnpackV2_16 << kVifCmdShift) |
                (static_cast<unsigned>(nPrimCount) << (kVifNumShift - 1)) | kVifUnpackFlg |
                static_cast<unsigned>(nVertCount * kVertQuadwordStride + nVertCount +
                                      kVu1EdgeVertAddr));
        AppendIndexData(run.mIndices, run.mIndexCount);

        g_gfxDevice.FlushReservedGif();
        GifQuadword *pEntry = TakeQuadword();
        pEntry->mHi = 0;
        if (&run == &pOwner->mEdgeRuns.front()) {
            pEntry->mLo = MakeVifCode(kVifCmdMsCal, 0, kVu1EdgeEntry);
        } else {
            pEntry->mLo = MakeVifCode(kVifCmdMsCnt, 0, 0);
        }
        g_gfxDevice.FlushGifPacket(0, 0);
    }
}

// 0x00601410
void PsMesh::DrawFacesSoftware(int nClip) {
    const int nTextured = g_nStageTextureBound;
    const unsigned long long qwPrim =
        kGsPrimTriangle |
        (static_cast<unsigned long long>(g_nSelectedFlat ^ 1) << kGsPrimIipShift) |
        (static_cast<unsigned long long>(nTextured) << kGsPrimTmeShift) |
        (static_cast<unsigned long long>(g_nFogEnabled) << kGsPrimFgeShift) |
        (static_cast<unsigned long long>(g_nAlphaBlendEnabled) << kGsPrimAbeShift);
    g_gfxDevice.SetGsReg(kGsRegPrim, qwPrim, kGsPrimFieldMask);

    GifQuadword tag;
    if (nTextured != 0) {
        tag.mLo = static_cast<unsigned long long>(kGifNRegTexturedTri) << kGifTagNRegShift;
        tag.mHi = kGifRegsTexturedTri;
    } else {
        tag.mLo = static_cast<unsigned long long>(kGifNRegUntexturedTri) << kGifTagNRegShift;
        tag.mHi = kGifRegsUntexturedTri;
    }
    g_gfxDevice.WriteGifTag(&tag);

    for (auto &face : mFacesOwner->mFaces) {
        EmitTriangle(face.mV1, face.mV2, face.mV3, nClip);
        g_gfxDevice.FlushGifPacket(1, 1);
    }
}

// 0x006017c0
void PsMesh::DrawEdgesSoftware(int nClip) {
    const unsigned long long qwPrim =
        kGsPrimLine | kGsPrimAa1 |
        (static_cast<unsigned long long>(g_nFogEnabled) << kGsPrimFgeShift);
    g_gfxDevice.SetGsReg(kGsRegPrim, qwPrim, kGsPrimFieldMask);

    GifQuadword tag;
    tag.mLo = static_cast<unsigned long long>(kGifNRegLine) << kGifTagNRegShift;
    tag.mHi = kGifRegsLine;
    g_gfxDevice.WriteGifTag(&tag);

    // Every edge of the mesh draws in one colour, so the packed RGBAQ quadword is built once.
    Color edgeColor;
    if (mMat != nullptr) {
        edgeColor = mMat->mSpecular;
    } else {
        edgeColor.r = 1.0f;
        edgeColor.g = 1.0f;
        edgeColor.b = 1.0f;
        edgeColor.a = 1.0f;
    }
    GifQuadword rgbaq;
    rgbaq.mLo = PackWordPair(static_cast<unsigned>(static_cast<int>(edgeColor.r * kGsColorScale)),
                             static_cast<unsigned>(static_cast<int>(edgeColor.g * kGsColorScale)));
    rgbaq.mHi = PackWordPair(static_cast<unsigned>(static_cast<int>(edgeColor.b * kGsColorScale)),
                             static_cast<unsigned>(static_cast<int>(edgeColor.a * kGsAlphaScale)));

    for (auto &edge : mFacesOwner->mEdges) {
        if (nClip != 0) {
            const int nFlags =
                g_aDrawVerts[edge.mV1].mClipFlags | g_aDrawVerts[edge.mV2].mClipFlags;
            if ((nFlags & kDrawVertClipAnyPlane) != 0) {
                // An edge is dropped rather than clipped, which is why a long edge crossing the
                // view plane disappears instead of being shortened.
                ++g_renderStats.mnEdgesClipped;
                g_gfxDevice.FlushGifPacket(1, 1);
                continue;
            }
        }
        GifQuadword *pColor = TakeQuadword();
        *pColor = rgbaq;
        AppendDrawVert(g_aDrawVerts[edge.mV1], 2, kDrawVertQuadwordsLine);
        AppendDrawVert(g_aDrawVerts[edge.mV2], 2, kDrawVertQuadwordsLine);
        ++g_renderStats.mnLines;
        g_gfxDevice.FlushGifPacket(1, 1);
    }
}

// 0x006069a0
inline void PsMesh::DrawRun::ReserveIndices(int nIndexCount) {
    // The binary rounds with a plain arithmetic shift rather than a signed division.
    const int nQuadwords = (nIndexCount + kIndexHalfwordsPerQuadword - 1) >> kIndexHalfwordShift;
    if (mIndexCount < nQuadwords) {
        mIndexCount = nIndexCount;
        if (mIndices != nullptr) {
            MemFree(mIndices);
        }
        mIndices = static_cast<unsigned short *>(
            MemAlloc(static_cast<size_t>(nQuadwords) * sizeof(GifQuadword)));
    }
}

// The compiler inlined this at both of its call sites in Sync().
void PsMesh::AppendRun(std::list<DrawRun> &runs,
                       const std::vector<unsigned short> &vertIndices,
                       const std::vector<unsigned short> &primIndices) {
    DrawRun empty;
    empty.mIndices = nullptr;
    empty.mIndexCount = 0;
    runs.push_back(empty);

    DrawRun &run = runs.back();
    run.mVertIndices = vertIndices;

    // A node arrives here with a count of zero, so the block is always allocated once.
    run.ReserveIndices(static_cast<int>(primIndices.size()));

    // The block travels a whole quadword at a time, so the padding past the last index is cleared
    // rather than sent as whatever the allocator returned.
    const int nBlockQuadwords =
        (run.mIndexCount + kIndexHalfwordsPerQuadword - 1) / kIndexHalfwordsPerQuadword;
    GifQuadword *pBlock = AsWritableQuadwords(run.mIndices);
    pBlock[nBlockQuadwords - 1].mLo = 0;
    pBlock[nBlockQuadwords - 1].mHi = 0;
    std::copy(primIndices.begin(), primIndices.end(), run.mIndices);
}

// 0x00600590
void PsMesh::Sync() {
    // Both scratch lists start at the largest run either pass can produce and are emptied before
    // each run, so no run reallocates them.
    std::vector<unsigned short> vertIndices(kRunVertCapacity, 0);
    std::vector<unsigned short> primIndices(kRunIndexCapacity, 0);

    PsMesh *pOwner = static_cast<PsMesh *>(mFacesOwner);

    pOwner->mFaceRuns.clear();
    unsigned nFace = 0;
    while (nFace < pOwner->mFaces.size()) {
        int nBudget = kFaceRunQuadwordBudget;
        vertIndices.resize(0);
        primIndices.resize(0);
        for (;;) {
            const MeshFace &face = pOwner->mFaces[nFace];
            // The corners are visited from the second, which the binary expresses as a modulo
            // over the three and which the draw paths match when they submit a triangle.
            const unsigned short anCorners[] = {face.mV2, face.mV3, face.mV1};
            int nPrimVerts = 0;
            bool bOutOfRoom = false;
            for (int nCorner = 0; nCorner < kFaceIndicesPerPrim; ++nCorner) {
                auto found = std::find(vertIndices.begin(), vertIndices.end(), anCorners[nCorner]);
                if (found != vertIndices.end()) {
                    primIndices.push_back(static_cast<unsigned short>(
                        (found - vertIndices.begin()) * kVu1VertStride));
                    continue;
                }
                if (nBudget < kFaceRunReserve) {
                    // Un-do the part of this face already recorded and reprocess it at the head of
                    // the next run.
                    primIndices.resize(primIndices.size() - nCorner);
                    vertIndices.resize(vertIndices.size() - nPrimVerts);
                    bOutOfRoom = true;
                    break;
                }
                vertIndices.push_back(anCorners[nCorner]);
                ++nPrimVerts;
                nBudget -= kFaceVertQuadwordCost;
                primIndices.push_back(
                    static_cast<unsigned short>((vertIndices.size() - 1) * kVu1VertStride));
            }
            if (bOutOfRoom) {
                break;
            }
            --nBudget;
            ++nFace;
            if (nBudget == 0 || nFace == pOwner->mFaces.size()) {
                break;
            }
        }
        AppendRun(pOwner->mFaceRuns, vertIndices, primIndices);
    }

    pOwner->mEdgeRuns.clear();
    unsigned nEdge = 0;
    while (nEdge < pOwner->mEdges.size()) {
        int nBudget = kEdgeRunQuadwordBudget;
        vertIndices.resize(0);
        primIndices.resize(0);
        for (;;) {
            const MeshEdge &edge = pOwner->mEdges[nEdge];
            const unsigned short anCorners[] = {edge.mV1, edge.mV2};
            int nPrimVerts = 0;
            bool bOutOfRoom = false;
            for (int nCorner = 0; nCorner < kEdgeIndicesPerPrim; ++nCorner) {
                auto found = std::find(vertIndices.begin(), vertIndices.end(), anCorners[nCorner]);
                if (found != vertIndices.end()) {
                    primIndices.push_back(static_cast<unsigned short>(
                        (found - vertIndices.begin()) * kVu1VertStride));
                    continue;
                }
                if (nBudget < kEdgeRunReserve) {
                    primIndices.resize(primIndices.size() - nCorner);
                    vertIndices.resize(vertIndices.size() - nPrimVerts);
                    bOutOfRoom = true;
                    break;
                }
                vertIndices.push_back(anCorners[nCorner]);
                ++nPrimVerts;
                nBudget -= kEdgeVertQuadwordCost;
                primIndices.push_back(
                    static_cast<unsigned short>((vertIndices.size() - 1) * kVu1VertStride));
            }
            if (bOutOfRoom) {
                break;
            }
            --nBudget;
            ++nEdge;
            if (nBudget == 0 || nEdge == pOwner->mEdges.size()) {
                break;
            }
        }
        AppendRun(pOwner->mEdgeRuns, vertIndices, primIndices);
    }
}

// 0x00606928
Mesh *NewPsMesh(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::Mesh" and rounds the 0x170-byte object up
    // to 0x180 bytes.
    return new PsMesh(name);
}

} // namespace Rnd
