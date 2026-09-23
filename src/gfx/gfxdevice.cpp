#include "gfx/gfxdevice.h"

#include <cstdint>
#include <eekernel.h>
#include <libdma.h>
#include <libgraph.h>

#include "app/longop.h"
#include "gfx/gsdoublebuffer.h"
#include "gfx/renderstats.h"
#include "gfx/vramtable.h"

namespace {

// GIFtag fields the packet routines touch, all within the tag's low 64 bits. NLOOP occupies the
// low fifteen bits, EOP the next one, FLG bits 58 and 59, and NREG the top four.
constexpr unsigned long long kGifTagNLoopMask = 0x7fff;
constexpr int kGifTagEopShift = 15;
constexpr unsigned long long kGifTagEop = 1ULL << kGifTagEopShift;
constexpr int kGifTagFlgShift = 58;
constexpr unsigned long long kGifTagFlgMask = 3;
constexpr int kGifTagNRegShift = 60;

// The first register descriptor of a tag's REGS field, in the high 64 bits, and the value that
// makes the tag an A+D tag.
constexpr unsigned long long kGifTagFirstRegMask = 0xf;
constexpr unsigned long long kGifRegAd = 0xe;

// VIF codes the VU1 path places ahead of GIF data. FLUSHA waits for the paths to go idle, and
// DIRECT passes the quadword count in its immediate straight through to the GIF.
constexpr unsigned long long kVifCodeFlushA = 0x13000000;
constexpr unsigned long long kVifCodeDirect = 0x50000000;
constexpr int kVifCodeHighWordShift = 32;

// The two halves of the scratchpad the packet buffer alternates between.
constexpr std::uintptr_t kGifBufferHalf0 = 0x70000000;
constexpr std::uintptr_t kGifBufferHalf1 = 0x70002000;

// A buffer address becomes a DMA address by keeping the low 28 bits and moving the scratchpad
// selector, bit 30, up to bit 31, where the DMA controller reads it.
constexpr std::uintptr_t kDmaAddressMask = 0x0fffffff;
constexpr std::uintptr_t kScratchpadAddressBit = 0x40000000;

// PRIM and the primitive types that continue across vertices. A strip or a fan is restarted only
// by a PRIM write.
constexpr int kGsRegPrim = 0;
constexpr unsigned long long kGsPrimTypeMask = 7;
constexpr unsigned long long kGsPrimLineStrip = 2;
constexpr unsigned long long kGsPrimTriStrip = 4;
constexpr unsigned long long kGsPrimTriFan = 5;

// The context 1 registers RestoreFrameBufferTarget() restores, and the fields it sets. FRAME_1
// takes FBP, FBW, and PSM without FBMSK, and XYOFFSET_1 takes both offsets.
constexpr int kGsRegFrame1 = 0x4c;
constexpr int kGsRegXyOffset1 = 0x18;
constexpr unsigned long long kGsFrameMask = 0x3f3f01ff;
constexpr unsigned long long kGsXyOffsetMask = 0x0000ffff0000ffffULL;

// A GS page is 2048 words, and a word is four bytes.
constexpr int kGsPageWordShift = 11;
constexpr int kBytesPerWordShift = 2;

// Arguments to sceGsSyncPath() that wait for every path without a timeout.
constexpr int kGsSyncPathWait = 0;
constexpr unsigned short kGsSyncPathNoTimeout = 0;

// RGBAQ as the device packs a colour. Red, green, and blue scale to 255, alpha to 128, and Q is
// 1.0f in the upper word.
constexpr float kRgbaqColorScale = 255.0f;
constexpr float kRgbaqAlphaScale = 128.0f;
constexpr int kRgbaqGreenShift = 8;
constexpr int kRgbaqBlueShift = 16;
constexpr int kRgbaqAlphaShift = 24;
constexpr unsigned long long kRgbaqQOne = 0x3f800000ULL << 32;

// The value word of the RGBAQ pair, the third register pair of an sceGsClear.
constexpr int kClearRgbaqWord = 4;

// The write-back mode of FlushCache().
constexpr int kFlushCacheWriteBackData = 0;

// Registers and fields SetupGsDrawContext() programs.
constexpr int kGsRegTex0_1 = 0x06;
constexpr int kGsRegClamp1 = 0x08;
constexpr int kGsRegTex1_1 = 0x14;
constexpr int kGsRegTexA = 0x3b;
constexpr int kGsRegAlpha1 = 0x42;
constexpr int kGsRegDimX = 0x44;
constexpr int kGsRegTest1 = 0x47;
constexpr int kGsRegZbuf1 = 0x4e;
constexpr unsigned long long kAllBits = ~0ULL;
constexpr unsigned long long kTestZTestMask = 0x60000;
constexpr unsigned long long kTestZTestAlways = 0x20000;
constexpr unsigned long long kZbufZmsk = 1ULL << 32;
constexpr unsigned long long kAlphaMask = 0xff000000ffULL;
// A = Cs, B = Cd, C = FIX, and D = Cd. The source is blended over the destination by FIX.
constexpr unsigned long long kAlphaBlendByFix = 0x64;
constexpr int kAlphaFixShift = 32;
constexpr unsigned long long kDimXMatrix = 0x1212303012120303ULL;
constexpr unsigned long long kTex1MinMask = 0x1c0;
constexpr unsigned long long kTex1MinLinear = 0x40;
constexpr unsigned long long kClampMask = 0xf;
constexpr unsigned long long kClampBothAxes = 5;
constexpr unsigned long long kTexAHalfAlpha = 0x0000008000000080ULL;

// FRAME_1 fields the texture binding copies, and the TEX0_1 fields it fills. The texture is
// 1024 by 1024 with its alpha taken from the texture and applied as a decal.
constexpr unsigned long long kFrameFbpMask = 0x1ff;
constexpr int kFrameFbwShift = 16;
constexpr unsigned long long kFrameFbwMask = 0x3f;
constexpr int kFramePsmShift = 24;
constexpr unsigned long long kFramePsmMask = 0x3f;
constexpr int kBlocksPerPageShift = 5;
constexpr int kTex0TbwShift = 14;
constexpr int kTex0PsmShift = 20;
constexpr int kTex0TwShift = 26;
constexpr int kTex0ThShift = 30;
constexpr int kTex0TccShift = 34;
constexpr int kTex0TfxShift = 35;
constexpr unsigned long long kTex0Log2Size1024 = 10;
constexpr unsigned long long kTex0TccRgba = 1;
constexpr unsigned long long kTex0TfxDecal = 1;
constexpr unsigned long long kTex0Mask = 0xffffffffffULL;

// The feedback sprite travels under one REGLIST tag, PRIM and RGBAQ then two UV and XYZ2 pairs.
constexpr unsigned long long kFeedbackTagLo = 0x6400000000008000ULL;
constexpr unsigned long long kFeedbackTagHi = 0x535310;
// A textured, alpha-blended sprite with texel coordinates, drawn in mid grey at alpha 100.
constexpr unsigned long long kFeedbackPrim = 0x156;
constexpr unsigned long long kFeedbackRgbaq = 0x3f80000064808080ULL;
constexpr unsigned long long kFeedbackZ = 160000ULL << 32;
// Coordinates are in sixteenths. The primitive origin is 2048, and a texel is sampled at its
// centre.
constexpr int kGsCoordinateCentre = 0x8000;
constexpr int kGsSubpixelShift = 4;
constexpr int kHalfSizeToSubpixelShift = 3;
constexpr int kTexelCentre = 8;
constexpr int kGsYShift = 16;

inline unsigned long long PackRgbaq(const Color &color) {
    const int nRed = static_cast<int>(color.r * kRgbaqColorScale);
    const int nGreen = static_cast<int>(color.g * kRgbaqColorScale);
    const int nBlue = static_cast<int>(color.b * kRgbaqColorScale);
    const int nAlpha = static_cast<int>(color.a * kRgbaqAlphaScale);
    return static_cast<unsigned long long>(nRed) |
           (static_cast<unsigned long long>(nGreen) << kRgbaqGreenShift) |
           (static_cast<unsigned long long>(nBlue) << kRgbaqBlueShift) |
           (static_cast<unsigned long long>(nAlpha) << kRgbaqAlphaShift) | kRgbaqQOne;
}

inline unsigned long long PackCoordinates(int nX, int nY) {
    return static_cast<unsigned long long>(nX) | (static_cast<unsigned long long>(nY) << kGsYShift);
}

} // namespace

// 0x0049b478
int GfxDevice::FlushGifPacket(int bRetainOpenTag, int bOnlyWhenFull) {
    if (bOnlyWhenFull != 0 && mpWrite < mpBuffer + kGifBufferQuadwords) {
        return 0;
    }
    if (mpWrite == mpBuffer) {
        return 0;
    }

    ++g_renderStats.mnGifPackets;
    GifQuadword retainedTag;
    if (bRetainOpenTag != 0) {
        retainedTag = *mpOpenTag;
    }
    CloseGifTag(1);

    sceDmaChan *pChannel = sceDmaGetChan(mnUseVu1 != 0 ? SCE_DMA_VIF1 : SCE_DMA_GIF);
    const std::uintptr_t nBuffer = reinterpret_cast<std::uintptr_t>(mpBuffer);
    const std::uintptr_t nDmaAddress =
        (nBuffer & kDmaAddressMask) | ((nBuffer & kScratchpadAddressBit) << 1);
    // The DMA address is a bus address the processor cannot dereference, but sceDmaSendN() takes
    // it as a pointer.
    sceDmaSendN(
        pChannel, reinterpret_cast<void *>(nDmaAddress), static_cast<int>(mpWrite - mpBuffer));
    g_vramTable.AdvanceLockCycle();

    if (reinterpret_cast<std::uintptr_t>(mpBuffer) != kGifBufferHalf0) {
        mpBuffer = reinterpret_cast<GifQuadword *>(kGifBufferHalf0);
    } else {
        mpBuffer = reinterpret_cast<GifQuadword *>(kGifBufferHalf1);
    }
    mpWrite = mpBuffer;

    if (bRetainOpenTag != 0) {
        WriteGifTag(&retainedTag);
    }
    RunLongOperationPollProc();
    return 1;
}

// 0x0049b5a8
void GfxDevice::WriteGifTag(const GifQuadword *pTag) {
    ++g_renderStats.mnGifTags;
    CloseGifTag(0); // Inlined in the binary.

    if (mnUseVu1 != 0 && mpOpenVifDirect == nullptr) {
        GifQuadword *pCodes = mpWrite;
        mpWrite = pCodes + 1;
        // Two VIF NOPs, then FLUSHA, then a DIRECT whose count CloseGifTag() fills in.
        pCodes->mLo = 0;
        pCodes->mHi = (kVifCodeDirect << kVifCodeHighWordShift) | kVifCodeFlushA;
        mpOpenVifDirect = mpWrite;
    }

    GifQuadword *pWrite = mpWrite;
    mpOpenTag = pWrite;
    *pWrite = *pTag;
    mpWrite = pWrite + 1;
}

// 0x0049ffd0
void GfxDevice::SetGsReg(int nReg, unsigned long long qwValue, unsigned long long qwMask) {
    if ((mGsRegs[nReg] & qwMask) == (qwValue & qwMask)) {
        return;
    }

    if (mpOpenTag == nullptr || (mpOpenTag->mHi & kGifTagFirstRegMask) != kGifRegAd) {
        WriteGifTag(&mAdTag);
    }
    const unsigned long long qwMerged = (mGsRegs[nReg] & ~qwMask) | (qwValue & qwMask);
    mGsRegs[nReg] = qwMerged;
    GifQuadword *pWrite = mpWrite;
    mpWrite = pWrite + 1;
    pWrite->mLo = qwMerged;
    pWrite->mHi = static_cast<unsigned long long>(nReg);
    FlushGifPacket(0, 1);

    if (nReg != kGsRegPrim) {
        return;
    }
    const unsigned long long qwPrimType = mGsRegs[kGsRegPrim] & kGsPrimTypeMask;
    if (qwPrimType == kGsPrimTriStrip || qwPrimType == kGsPrimTriFan ||
        qwPrimType == kGsPrimLineStrip) {
        mGsRegs[kGsRegPrim] |= kGsPrimTypeMask;
    }
}

// 0x004a0348
void GfxDevice::EnterVu1Path() {
    if (mnUseVu1 != 0) {
        return;
    }
    FlushGifPacket(0, 0);
    mnUseVu1 = 1;
}

// 0x0049b838
void GfxDevice::LeaveVu1Path() {
    if (mnUseVu1 == 0) {
        return;
    }
    CloseGifTag(1); // Inlined in the binary.
    FlushGifPacket(0, 0);
    sceGsSyncPath(kGsSyncPathWait, kGsSyncPathNoTimeout);
    mnUseVu1 = 0;
}

// 0x0049fd98
void GfxDevice::ReserveGifSpace(int nQuadwords) {
    if (mnUseVu1 == 0) {
        return;
    }
    if (mpReservedRegion != nullptr) {
        return;
    }
    CloseGifTag(1);
    GifQuadword *pRegion = mpBuffer + (kGifBufferQuadwords - nQuadwords);
    mpSavedWrite = mpWrite;
    mpReservedRegion = pRegion;
    mpWrite = pRegion;
}

// 0x0049fe08
void GfxDevice::SwapGifWrite() {
    if (mnUseVu1 == 0) {
        return;
    }
    if (mpReservedRegion == nullptr) {
        return;
    }
    GifQuadword *pWrite = mpWrite;
    mpWrite = mpSavedWrite;
    mpSavedWrite = pWrite;
}

// 0x0049fe38
void GfxDevice::FlushReservedGif() {
    if (mnUseVu1 == 0) {
        return;
    }
    if (mpReservedRegion == nullptr) {
        return;
    }
    // A caller arrives here having swapped back to the main stream. That puts the region's write
    // pointer in mpSavedWrite, and its distance from the region start is the amount written.
    const int nQuadwords = static_cast<int>(mpSavedWrite - mpReservedRegion);
    const GifQuadword *pSource = mpReservedRegion;
    for (int nRemaining = nQuadwords; nRemaining > 0; --nRemaining) {
        GifQuadword *pDest = mpWrite;
        *pDest = *pSource;
        ++pSource;
        mpWrite = pDest + 1;
    }
    mpReservedRegion = nullptr;
}

// 0x004a0158
void GfxDevice::CloseGifTag(int bEndOfPacket) {
    GifQuadword *pTag = mpOpenTag;
    if (pTag == nullptr) {
        return;
    }

    const unsigned long long qwTag = pTag->mLo;
    const int nQuadwords = static_cast<int>(mpWrite - pTag);
    // NREG shifted right by FLG is the register count one loop consumes. A packed loop spends a
    // quadword per register, a register list one per two registers, and an image one per four.
    const unsigned int nFlg =
        static_cast<unsigned int>((qwTag >> kGifTagFlgShift) & kGifTagFlgMask);
    const unsigned int nPerLoop = static_cast<unsigned int>(qwTag >> kGifTagNRegShift) >> nFlg;
    const unsigned long long qwNLoop =
        static_cast<unsigned long long>(nQuadwords - 1) / nPerLoop & kGifTagNLoopMask;
    pTag->mLo = (qwTag & ~kGifTagNLoopMask) | qwNLoop;

    // The binary re-reads the tag pointer here rather than reusing the one above.
    GifQuadword *pSameTag = mpOpenTag;
    pSameTag->mLo = (pSameTag->mLo & ~kGifTagEop) |
                    (static_cast<unsigned long long>(bEndOfPacket & 1) << kGifTagEopShift);
    mpOpenTag = nullptr;

    if (bEndOfPacket == 0 || mnUseVu1 == 0) {
        return;
    }
    // The DIRECT code sits in the high word of the quadword before the data it introduces.
    const unsigned long long qwDirectCount =
        static_cast<unsigned long long>(mpWrite - mpOpenVifDirect);
    mpOpenVifDirect[-1].mHi |= qwDirectCount << kVifCodeHighWordShift;
    mpOpenVifDirect = nullptr;
}

// 0x0049b6b8
void GfxDevice::RestoreFrameBufferTarget() {
    const sceGsDrawEnv1 &draw =
        mnDrawBuffer != 0 ? mpDisplayBuffers->mHalves[1].mDraw : mpDisplayBuffers->mHalves[0].mDraw;
    SetGsReg(kGsRegFrame1, draw.frame1, kGsFrameMask);
    SetGsReg(kGsRegXyOffset1, draw.xyoffset1, kGsXyOffsetMask);
}

// 0x0049b368
void GfxDevice::SetClearColor(const Color &color) {
    mClearColor = color;
    mpDisplayBuffers->mHalves[0].mClear.mWords[kClearRgbaqWord] = PackRgbaq(color);
    mpDisplayBuffers->mHalves[1].mClear.mWords[kClearRgbaqWord] = PackRgbaq(color);
    FlushCache(kFlushCacheWriteBackData);
}

// 0x0049ccb8
void GfxDevice::SetupGsDrawContext() {
    SetGsReg(kGsRegTest1, kTestZTestAlways, kTestZTestMask);
    SetGsReg(kGsRegZbuf1, kZbufZmsk, kZbufZmsk);
    SetGsReg(
        kGsRegAlpha1,
        (static_cast<unsigned long long>(mFeedbackAlpha * kRgbaqAlphaScale) << kAlphaFixShift) |
            kAlphaBlendByFix,
        kAlphaMask);
    SetGsReg(kGsRegDimX, kDimXMatrix, kAllBits);

    // The texture is the frame buffer of the half not being drawn.
    const unsigned long long qwFrame = mnDrawBuffer == 0 ?
                                           mpDisplayBuffers->mHalves[1].mDraw.frame1 :
                                           mpDisplayBuffers->mHalves[0].mDraw.frame1;
    // The binary builds TEX0_1 over an uninitialised register, so bits 37 to 39 inside the mask
    // are undefined there. They are zero here.
    const unsigned long long qwTex0 =
        ((qwFrame & kFrameFbpMask) << kBlocksPerPageShift) |
        (((qwFrame >> kFrameFbwShift) & kFrameFbwMask) << kTex0TbwShift) |
        (((qwFrame >> kFramePsmShift) & kFramePsmMask) << kTex0PsmShift) |
        (kTex0Log2Size1024 << kTex0TwShift) | (kTex0Log2Size1024 << kTex0ThShift) |
        (kTex0TccRgba << kTex0TccShift) | (kTex0TfxDecal << kTex0TfxShift);
    SetGsReg(kGsRegTex0_1, qwTex0, kTex0Mask);
    SetGsReg(kGsRegTex1_1, kTex1MinLinear, kTex1MinMask);
    SetGsReg(kGsRegClamp1, kClampBothAxes, kClampMask);
    SetGsReg(kGsRegTexA, kTexAHalfAlpha, kAllBits);

    GifQuadword tag;
    tag.mLo = kFeedbackTagLo;
    tag.mHi = kFeedbackTagHi;
    WriteGifTag(&tag);
    GifQuadword *pPrim = mpWrite;
    mpWrite = pPrim + 1;
    pPrim->mHi = kFeedbackRgbaq;
    pPrim->mLo = kFeedbackPrim;

    const float flWidth = static_cast<float>(mnDisplayWidth);
    const float flHeight = static_cast<float>(mnDisplayHeight);
    const int nLeft = kGsCoordinateCentre - (mnDisplayWidth << kHalfSizeToSubpixelShift);
    const int nTop = kGsCoordinateCentre - (mnDisplayHeight << kHalfSizeToSubpixelShift);

    const int nNearY = static_cast<int>(mFeedbackRect.g * flHeight) << kGsSubpixelShift;
    const int nNearX = static_cast<int>(mFeedbackRect.r * flWidth) << kGsSubpixelShift;
    GifQuadword *pNear = mpWrite;
    mpWrite = pNear + 1;
    pNear->mLo = PackCoordinates(nNearX + mFeedbackInset + kTexelCentre,
                                 nNearY + mFeedbackInset + kTexelCentre);
    pNear->mHi = PackCoordinates(nLeft + nNearX, nTop + nNearY) | kFeedbackZ;

    const int nFarY = static_cast<int>((mFeedbackRect.g + mFeedbackRect.a) * flHeight)
                      << kGsSubpixelShift;
    const int nFarX = static_cast<int>((mFeedbackRect.r + mFeedbackRect.b) * flWidth)
                      << kGsSubpixelShift;
    GifQuadword *pFar = mpWrite;
    mpWrite = pFar + 1;
    pFar->mHi = PackCoordinates(nLeft + nFarX, nTop + nFarY) | kFeedbackZ;
    pFar->mLo = PackCoordinates(nFarX - mFeedbackInset + kTexelCentre,
                                nFarY - mFeedbackInset + kTexelCentre);
    FlushGifPacket(0, 1);
}

// 0x0049fec0
int GfxDevice::GetReservedVramWords() const {
    return (mpDisplayBuffers->mZbp << kGsPageWordShift) +
           ((mnDisplayWidth * mnDisplayHeight * mnDepthBytes) >> kBytesPerWordShift);
}
