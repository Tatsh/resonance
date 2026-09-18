#include <vector>

#include "os/async.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/tex.h"

namespace Rnd {

namespace {

// The tag every release in this file bills to, and the file name its asserts record.
constexpr char kTexFileName[] = "rndtex.cpp";

// Line 610 of rndtex.cpp, which FreeLoadedBitmaps() passes to the tagged release.
constexpr int kFreeBitmapLine = 0x262;

} // namespace

// 0x004e3dc8
Tex::Tex(const HxStr &name)
    : Object(name), mUnknown1c(0), mUnknown20(0), mUnknown24(0), mUnknown28(0), mPendingMipMask(0),
      mMipSelect(-0x80), mBitmapPath(nullptr), mGsHandle(-1) {
}

// 0x004e7878
bool Tex::IsLoadComplete() {
    return mPendingMipMask == 0;
}

// 0x004e4410
bool Tex::PollAsyncMips() {
    if (mPendingMipMask == 0) {
        return true;
    }

    for (unsigned nMip = 0; nMip < mMipHandles.size(); ++nMip) {
        if (((mPendingMipMask >> nMip) & 1) == 0) {
            continue;
        }

        void *pBitmap = nullptr;
        const int nStatus = AsyncPollComplete(mMipHandles[nMip], &pBitmap, nullptr);
        if (nStatus == 0) {
            mLoadedBitmaps[nMip] = pBitmap;
            OnMipLoaded();
            mPendingMipMask &= ~(1 << nMip);
            continue;
        }
        if (nStatus > 0) {
            g_failSink.Report(
                "Texture %s mip %d: async read error %d\n", mBitmapPath.mStr, nMip, nStatus);
            // A failed read clears its bit and reports the load complete, which stops the caller
            // spinning on a mip that will never arrive.
            mPendingMipMask &= ~(1 << nMip);
            return true;
        }
    }

    if (mPendingMipMask != 0) {
        return false;
    }
    OnAllMipsLoaded();
    return true;
}

// 0x004e7aa8
void Tex::FreeLoadedBitmaps() {
    CancelPendingMips();
    for (const auto pBitmap : mLoadedBitmaps) {
        // A bitmap already resident in GS memory belongs to its slot, so only a copy that never
        // arrived there is released here.
        if (pBitmap != nullptr && mGsHandle == -1) {
            MemFreeTagged(pBitmap, kTexFileName, kFreeBitmapLine);
        }
    }
    mLoadedBitmaps.clear();
}

} // namespace Rnd
