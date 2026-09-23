#include "gfx/gfxdevice.h"

#include <cstdint>
#include <libdma.h>
#include <libgraph.h>

#include "app/longop.h"
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

// Arguments to sceGsSyncPath() that wait for every path without a timeout.
constexpr int kGsSyncPathWait = 0;
constexpr unsigned short kGsSyncPathNoTimeout = 0;

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
