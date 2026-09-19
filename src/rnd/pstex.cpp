#include "rnd/pstex.h"

#include "os/hxstr.h"
#include "os/mem.h"

namespace Rnd {

// 0x00596f80
PsTex::PsTex(const HxStr &name)
    : Tex(name), mUnknown4a0(nullptr), mUnknown4a4(0), mPaletteVram(nullptr) {
}

// 0x0059a558
PsTex::~PsTex() {
    FreeGsSurfaces();
    // The guard is the null check the compiler emits for a scalar delete, and the release is the
    // scalar path rather than the array path.
    if (mUnknown4a0 != nullptr) {
        MemFreeScalar(mUnknown4a0);
    }
}

// 0x0059a7e8
void PsTex::FreeLoadedBitmaps() {
    FreeGsSurfaces();
    Tex::FreeLoadedBitmaps();
}

// 0x0059ab30
void PsTex::UnlockMipBitmap() {
    if (mLoadedBitmaps.empty()) {
        return;
    }
    // Yes, the binary indexes by the recorded level without range-checking it against the vector.
    if (mLoadedBitmaps[mLockedMip] == nullptr) {
        return;
    }
    mDirtyMips |= 1u << mLockedMip;
}

} // namespace Rnd
