#include "rnd/psparticlesys.h"

#include "gfx/gfxdevice.h"
#include "gfx/renderstats.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/drawverts.h"
#include "rnd/particle.h"
#include "rnd/particlesys.h"
#include "rnd/psmat.h"

namespace Rnd {

namespace {

// GS general-purpose register indices, and the PRIM fields both software emitters program. The
// shipped build took the register numbers from the PlayStation 2 SDK headers.
constexpr int kGsRegPrim = 0;
constexpr int kGsRegTest1 = 0x47;
constexpr int kGsRegZbuf1 = 0x4e;
constexpr unsigned kGsPrimPoint = 0;
constexpr unsigned kGsPrimLine = 1;
constexpr unsigned kGsPrimSprite = 6;
constexpr int kGsPrimIipShift = 3;
constexpr int kGsPrimTmeShift = 4;
constexpr int kGsPrimAbeShift = 6;
constexpr unsigned kGsPrimIip = 1u << kGsPrimIipShift;
constexpr unsigned kGsPrimAbe = 1u << kGsPrimAbeShift;
constexpr unsigned long long kGsPrimFieldMask = 0x7ff;

// ZBUF_1.ZMSK, which suppresses the depth write, and TEST_1.ZTST.
constexpr int kZbufZMaskShift = 32;
constexpr unsigned long long kZbufZMaskField = 1ULL << kZbufZMaskShift;
constexpr int kTestZTestShift = 17;
constexpr unsigned long long kTestZTestField = 3ULL << kTestZTestShift;
constexpr int kGsZTestAlways = 1;
constexpr int kGsZTestGreater = 3;

// GIFtag NREG and its register descriptors, four bits each from the low nibble upward. Every
// packet here is the PACKED format, so FLG stays zero and one register spends one quadword.
constexpr int kGifTagNRegShift = 60;
constexpr unsigned long long kGifTagEop = 1ULL << 15;
constexpr unsigned long long kGifRegsPoint = 0x41ULL;
constexpr unsigned long long kGifRegsLine = 0x4141ULL;
constexpr unsigned long long kGifRegsTexturedSprite = 0x412412ULL;
constexpr unsigned long long kGifRegsUntexturedSprite = 0x441ULL;
constexpr int kGifNRegPoint = 2;
constexpr int kGifNRegLine = 4;
constexpr int kGifNRegTexturedSprite = 6;
constexpr int kGifNRegUntexturedSprite = 3;

// Quadwords of a Rnd::DrawVert each emitter sends, and where in the record it starts. A point and
// a line vertex send the colour and the position; a textured sprite vertex adds the texture
// coordinate ahead of them and an untextured far corner sends its position alone.
constexpr int kDrawVertColorQuadword = 1;
constexpr int kDrawVertPosQuadword = 2;
constexpr int kQuadwordsColorAndPos = 2;
constexpr int kQuadwordsWholeVert = 3;
constexpr int kQuadwordsPosOnly = 1;

// Packed vertices one particle occupies in the shared draw buffer. A point takes one and the other
// two modes take a near and a far corner.
constexpr int kVertsPerPointParticle = 1;
constexpr int kVertsPerPairedParticle = 2;

// Screen bounds the sprite emitter rejects against, in the fixed-point units the GS takes.
constexpr int kSpriteMinCoord = 0;
constexpr int kSpriteMaxCoord = 0xffff;

// Live particles a draw may submit through the software path. Exceeding it is reported rather than
// clamped, which matches Rnd::PsMesh.
constexpr int kMaxSoftwareParticles = 1250;

// Quadwords of GIF space a pass reserves before it programs any register.
constexpr int kGifReserveQuadwords = 32;

// VIFcode fields the vector unit path assembles. These repeat the set src/rnd/psmesh.cpp declares
// file-locally, because the two draw paths build their own codes and no shared header for the
// encoding exists yet.
constexpr unsigned kVifCmdMsCal = 0x14;
constexpr unsigned kVifCmdMsCnt = 0x17;
constexpr unsigned kVifCmdUnpackV4_32 = 0x6c;
constexpr int kVifCmdShift = 24;
constexpr int kVifNumShift = 16;
constexpr unsigned kVifUnpackFlg = 0x8000;

// VU1 data address a sprite batch unpacks to, and the microprogram it enters.
constexpr int kVu1SpriteBatchAddr = 9;
constexpr int kVu1SpriteEntry = 0x258;

// A batch closes at whichever of these two limits it arrives at first. The particle ceiling is the
// microprogram's own working set and the quadword ceiling is the eight-bit VIFcode NUM field.
constexpr int kVu1ParticlesPerBatch = 0xa2;
constexpr int kVifUnpackQuadwordLimit = 0xfe;

// Quadwords of a Rnd::Particle the vector unit path uploads, the colour then the position.
constexpr int kParticleUploadQuadwords = 2;

// Half, which the upload writes into the fourth word of the position because a GS sprite takes a
// centre and a half extent rather than two corners.
constexpr float kSpriteHalfExtent = 0.5f;

inline unsigned MakeVifCode(unsigned nCmd, int nNum, unsigned nImmediate) {
    return (nCmd << kVifCmdShift) | (static_cast<unsigned>(nNum) << kVifNumShift) | nImmediate;
}

inline unsigned long long PackWordPair(unsigned nLow, unsigned nHigh) {
    return nLow | (static_cast<unsigned long long>(nHigh) << 32);
}

inline GifQuadword *TakeQuadword() {
    GifQuadword *pQuad = g_gfxDevice.mpWrite;
    g_gfxDevice.mpWrite = pQuad + 1;
    return pQuad;
}

// The packet buffer is untyped quadwords, so the one conversion into it lives here.
inline const GifQuadword *AsQuadwords(const void *pRecord) {
    return static_cast<const GifQuadword *>(pRecord);
}

// Append part of a packed vertex, one quadword at a time as the binary does.
inline void AppendDrawVert(const DrawVert &vert, int nFirstQuadword, int nQuadwords) {
    const GifQuadword *pSource = AsQuadwords(&vert) + nFirstQuadword;
    for (int i = 0; i < nQuadwords; ++i) {
        GifQuadword *pDest = g_gfxDevice.mpWrite;
        *pDest = pSource[i];
        g_gfxDevice.mpWrite = pDest + 1;
    }
}

// Start a PACKED GIFtag of the given register set. Every packet here sets end-of-packet up front
// and lets CloseGifTag() fill in the loop count.
inline void WritePackedGifTag(int nRegCount, unsigned long long qwRegs) {
    GifQuadword tag;
    tag.mLo = (static_cast<unsigned long long>(nRegCount) << kGifTagNRegShift) | kGifTagEop;
    tag.mHi = qwRegs;
    g_gfxDevice.WriteGifTag(&tag);
}

// Screen-space reject for one sprite. The near corner must not fall below the origin and the far
// corner must not pass the coordinate ceiling.
inline bool IsSpriteOffScreen(const DrawVert &nearCorner, const DrawVert &farCorner) {
    const int nNearX = static_cast<int>(nearCorner.mPos.mLo);
    const int nNearY = static_cast<int>(nearCorner.mPos.mLo >> 32);
    if (nNearX < kSpriteMinCoord || nNearY < kSpriteMinCoord) {
        return true;
    }
    const int nFarX = static_cast<int>(farCorner.mPos.mLo);
    const int nFarY = static_cast<int>(farCorner.mPos.mLo >> 32);
    return nFarX > kSpriteMaxCoord || nFarY > kSpriteMaxCoord;
}

} // namespace

// 0x005fcdf0
PsParticleSys::PsParticleSys(const HxStr &name) : ParticleSys(name) {
}

// 0x005ff878
PsParticleSys::~PsParticleSys() {
}

// 0x005ffaf0
inline void PsParticleSys::EmitGifPoints(int nVertCount) {
    g_renderStats.mnPoints += nVertCount;
    g_gfxDevice.SetGsReg(kGsRegPrim, kGsPrimPoint | kGsPrimAbe, kGsPrimFieldMask);
    WritePackedGifTag(kGifNRegPoint, kGifRegsPoint);
    for (int i = 0; i < nVertCount; i += kVertsPerPointParticle) {
        AppendDrawVert(g_aDrawVerts[i], kDrawVertColorQuadword, kQuadwordsColorAndPos);
        g_gfxDevice.FlushGifPacket(1, 1);
    }
}

// 0x005fcb18
int PsParticleSys::DrawSelf() {
    ++g_renderStats.mnMeshDraws;
    if (mLiveParticles == nullptr) {
        return 1;
    }

    if (mMode == kModeSprite) {
        g_gfxDevice.ReserveGifSpace(kGifReserveQuadwords);
    }
    g_gfxDevice.SetGsReg(kGsRegZbuf1, kZbufZMaskField, kZbufZMaskField);
    const int nZTest = mReadZ != 0 ? kGsZTestGreater : kGsZTestAlways;
    g_gfxDevice.SetGsReg(
        kGsRegTest1, static_cast<unsigned long long>(nZTest) << kTestZTestShift, kTestZTestField);

    int nMorePasses = 0;
    if (mMat != nullptr) {
        nMorePasses = static_cast<PsMat *>(mMat)->Select();
    } else {
        PsMat::SelectDefault();
    }

    if (g_gfxDevice.mnUseVu1 != 0) {
        // Only the sprite mode has a microprogram. A point or line system draws nothing at all on
        // the vector unit path.
        if (mMode != kModeSprite) {
            return 1;
        }
        DrawSpritesDmaKicked();
        while (nMorePasses != 0) {
            g_gfxDevice.ReserveGifSpace(kGifReserveQuadwords);
            nMorePasses = static_cast<PsMat *>(mMat)->Select();
            DrawSpritesDmaKicked();
        }
        return 1;
    }

    if (static_cast<int>(mParticlesOwner->mParticles.size()) > kMaxSoftwareParticles) {
        // The guard measures the pool of whichever system owns the particles, and the message
        // prints the size of this system's own pool. Faithful to the binary.
        g_failSink.Report("DrawShowing particle buffer overflow... %d\n", mParticles.size());
        if (g_failSink.mAbortProc != nullptr) {
            g_failSink.mAbortProc();
        }
    }

    const int nVertCount = PackParticleQuads(g_aDrawVerts, mMode, mLiveParticles, mLineLength);
    if (mMode == kModePoint) {
        EmitGifPoints(nVertCount);
        return 1;
    }
    if (mMode == kModeLine) {
        EmitGifLines(nVertCount);
        return 1;
    }
    if (mMode == kModeSprite) {
        EmitGifSprites(nVertCount);
    }
    return 1;
}

// 0x005fc570
void PsParticleSys::EmitGifLines(int nVertCount) {
    // The counter advances by the whole vertex count before any line is built, so it records what
    // was offered rather than what was drawn.
    g_renderStats.mnLines += nVertCount;
    g_gfxDevice.SetGsReg(kGsRegPrim, kGsPrimLine | kGsPrimIip | kGsPrimAbe, kGsPrimFieldMask);
    WritePackedGifTag(kGifNRegLine, kGifRegsLine);

    for (int i = 0; i < nVertCount; i += kVertsPerPairedParticle) {
        AppendDrawVert(g_aDrawVerts[i], kDrawVertColorQuadword, kQuadwordsColorAndPos);
        AppendDrawVert(g_aDrawVerts[i + 1], kDrawVertColorQuadword, kQuadwordsColorAndPos);
        g_gfxDevice.FlushGifPacket(1, 1);
    }
}

// 0x005fc6d0
void PsParticleSys::EmitGifSprites(int nVertCount) {
    const int nTextured = g_nStageTextureBound;
    const unsigned long long qwPrim =
        kGsPrimSprite | kGsPrimIip |
        (static_cast<unsigned long long>(nTextured) << kGsPrimTmeShift) |
        (static_cast<unsigned long long>(g_nAlphaBlendEnabled) << kGsPrimAbeShift);
    g_gfxDevice.SetGsReg(kGsRegPrim, qwPrim, kGsPrimFieldMask);

    if (nTextured != 0) {
        WritePackedGifTag(kGifNRegTexturedSprite, kGifRegsTexturedSprite);
    } else {
        WritePackedGifTag(kGifNRegUntexturedSprite, kGifRegsUntexturedSprite);
    }

    for (int i = 0; i < nVertCount; i += kVertsPerPairedParticle) {
        const DrawVert &nearCorner = g_aDrawVerts[i];
        const DrawVert &farCorner = g_aDrawVerts[i + 1];
        if (IsSpriteOffScreen(nearCorner, farCorner)) {
            // A rejected sprite does not offer the packet for submission, unlike a rejected mesh
            // edge, which does.
            ++g_renderStats.mnSpritesCulled;
            continue;
        }
        ++g_renderStats.mnSpritesDrawn;
        if (nTextured != 0) {
            AppendDrawVert(nearCorner, 0, kQuadwordsWholeVert);
            AppendDrawVert(farCorner, 0, kQuadwordsWholeVert);
        } else {
            AppendDrawVert(nearCorner, kDrawVertColorQuadword, kQuadwordsColorAndPos);
            AppendDrawVert(farCorner, kDrawVertPosQuadword, kQuadwordsPosOnly);
        }
        g_gfxDevice.FlushGifPacket(1, 1);
    }
}

// 0x005fc940
void PsParticleSys::DrawSpritesDmaKicked() {
    g_gfxDevice.CloseGifTag(1);
    g_gfxDevice.SwapGifWrite();
    EmitParticleVu1Setup();

    Particle *pParticle = mLiveParticles;
    if (pParticle == nullptr) {
        return;
    }

    bool bFirstBatch = true;
    while (pParticle != nullptr) {
        // The batch header reaches the hardware before the batch length is known, so its VIFcode
        // slot is reserved here and filled in once the particles have been counted.
        GifQuadword *pBatchCode = TakeQuadword();
        pBatchCode->mLo = 0;
        pBatchCode->mHi = 0;
        GifQuadword *pCount = TakeQuadword();
        pCount->mLo = 0;
        pCount->mHi = 0;

        GifQuadword *pWrite = g_gfxDevice.mpWrite;
        int nVuAddr = kVu1SpriteBatchAddr;
        int nBatchQuadwords = 1;
        int nEmitted = 0;
        while (pParticle != nullptr) {
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

            pParticle->mPos.w = pParticle->mSize * kSpriteHalfExtent;
            ++nEmitted;
            nBatchQuadwords += kParticleUploadQuadwords;
            pWrite[0] = *AsQuadwords(&pParticle->mCol);
            pWrite[1] = *AsQuadwords(&pParticle->mPos);
            pWrite += kParticleUploadQuadwords;

            // The advance runs whether or not the ceiling ends the batch, so the next batch
            // resumes at the particle after this one either way.
            Particle *pNext = pParticle->mNext;
            if (nEmitted >= kVu1ParticlesPerBatch) {
                pParticle = pNext;
                break;
            }
            pParticle = pNext;
        }

        pBatchCode->mHi = PackWordPair(0,
                                       MakeVifCode(kVifCmdUnpackV4_32,
                                                   nBatchQuadwords,
                                                   kVifUnpackFlg | static_cast<unsigned>(nVuAddr)));
        g_gfxDevice.mpWrite = pWrite;
        pCount->mLo = static_cast<unsigned>(nEmitted);
        g_renderStats.mnSpritesDrawn += nEmitted;

        g_gfxDevice.FlushReservedGif();
        GifQuadword *pEntry = TakeQuadword();
        pEntry->mHi = 0;
        if (bFirstBatch) {
            pEntry->mLo = MakeVifCode(kVifCmdMsCal, 0, kVu1SpriteEntry);
            bFirstBatch = false;
        } else {
            pEntry->mLo = MakeVifCode(kVifCmdMsCnt, 0, 0);
        }
        g_gfxDevice.FlushGifPacket(0, 0);
    }
}

// 0x005ffa78
ParticleSys *NewPsParticleSys(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::ParticleSys" and the object is 0x220 bytes,
    // the same size as the base, because the subclass adds no member.
    return new PsParticleSys(name);
}

} // namespace Rnd
