#include "gfx/gfxdevice.h"

namespace {

// GIFtag fields the packet routines touch, all within the tag's low 64 bits. NLOOP occupies the
// low fifteen bits, EOP the next one, FLG bits 58 and 59, and NREG the top four.
constexpr unsigned long long kGifTagNLoopMask = 0x7fff;
constexpr unsigned long long kGifTagEop = 1ULL << 15;
constexpr int kGifTagFlgShift = 58;
constexpr unsigned long long kGifTagFlgMask = 3;
constexpr int kGifTagNRegShift = 60;

} // namespace

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
    // A caller arrives here having swapped back to the main stream. That puts the region's own
    // write pointer in mpSavedWrite, and its distance from the region start is the amount written.
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
    // NREG shifted right by FLG is how many registers one loop consumes: a packed loop spends a
    // quadword per register, a register list two per quadword, an image four.
    const unsigned int nFlg =
        static_cast<unsigned int>((qwTag >> kGifTagFlgShift) & kGifTagFlgMask);
    const unsigned int nPerLoop = static_cast<unsigned int>(qwTag >> kGifTagNRegShift) >> nFlg;
    const unsigned long long qwNLoop =
        static_cast<unsigned long long>(nQuadwords - 1) / nPerLoop & kGifTagNLoopMask;
    pTag->mLo = (qwTag & ~kGifTagNLoopMask) | qwNLoop;

    // The binary re-reads the tag pointer here rather than reusing the one above.
    GifQuadword *pSameTag = mpOpenTag;
    if (bEndOfPacket != 0) {
        pSameTag->mLo = pSameTag->mLo | kGifTagEop;
    } else {
        pSameTag->mLo = pSameTag->mLo & ~kGifTagEop;
    }

    mpOpenTag = nullptr;
}
