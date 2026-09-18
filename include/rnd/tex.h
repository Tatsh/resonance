#pragma once

#include <vector>

#include "os/hxstr.h"
#include "rnd/object.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Texture, as a bitmap path plus the load state of its mip levels.
 *
 * `Q23Rnd3Tex` in the RTTI descriptor at `0x008ef140`, with `Rnd::Object` as its one public
 * non-virtual base at offset 0. The class factory allocates 0x58 bytes, so the texture's own
 * members occupy `+0x1c` through `+0x57`. The implementation file is `rndtex.cpp`, attested by its
 * own assert strings, and the bitmap header it includes is `C:/FREQ/src/rndartt/abitmap.h`.
 *
 * A texture owns no pixels. The mip levels load asynchronously into the handle vector, and the
 * hardware residency belongs to the PlayStation 2 subclass Rnd::PsTex, whose GS slot state extends
 * the object past `+0x4a8`.
 *
 * Recovery is partial. The three configuration words the loader passes to SetBitmapConfig() are
 * not yet identified, so they are recorded by offset.
 */
class Tex : public Object {
public:
    /**
     * Report whether every requested mip level has finished loading.
     *
     * @return True when no level is outstanding.
     * @ghidraAddress 0x004e7878
     */
    bool IsLoadComplete();

    /**
     * Point the texture at a bitmap and restart its load.
     *
     * Records the three configuration words and the mip selector, then either clears the current
     * bitmap or begins loading the new path. The GS associations and the mip handle vector are
     * dropped either way.
     *
     * @param nUnknown1c The first configuration word.
     * @param nUnknown20 The second configuration word.
     * @param nUnknown24 The third configuration word.
     * @param path The bitmap path.
     * @param nMipSelect The mip selector, which starts at -0x80.
     * @param nUnknown28 The fourth configuration word.
     * @ghidraAddress 0x004e7908
     */
    void SetBitmapConfig(int nUnknown1c,
                         int nUnknown20,
                         int nUnknown24,
                         const HxStr &path,
                         int nMipSelect,
                         int nUnknown28);

    /**
     * Release every loaded bitmap and drop the GS associations.
     *
     * A level whose handle is set while the GS handle is absent trips the assert at line 0x262 of
     * `rndtex.cpp`.
     *
     * @ghidraAddress 0x004e7aa8
     */
    void FreeLoadedBitmaps();

    /**
     * Forget the current bitmap path.
     *
     * @param path The path to store, normally empty.
     * @ghidraAddress 0x004e7b70
     */
    void ClearBitmap(const HxStr &path);

    /**
     * Advance the asynchronous mip loads and report whether they have all arrived.
     *
     * @return True once no level is outstanding.
     * @ghidraAddress 0x004e4410
     */
    bool PollAsyncMips();

    /**
     * Allocate the bitmap of the next mip level from an open stream.
     *
     * @ghidraAddress 0x004e3fe8
     */
    void AllocateBitmapFromStream();

    /**
     * Begin loading the bitmap at a path.
     *
     * @param path The bitmap path.
     * @ghidraAddress 0x004e4bd8
     */
    void LoadBitmapFromPath(const HxStr &path);

protected:
    // Every member is protected rather than private, because Rnd::PsTex reads the mip handles, the
    // pending mask, and the bitmap path while it uploads to GS memory. No access from outside the
    // hierarchy is recovered. The order below is the recovered offset order.
    int mUnknown1c;                  // +0x1c
    int mUnknown20;                  // +0x20
    int mUnknown24;                  // +0x24
    int mUnknown28;                  // +0x28
    std::vector<int> mMipHandles;    // +0x2c
    unsigned char mPendingMipMask;   // +0x38 One bit per mip level still loading.
    int mMipSelect;                  // +0x3c Starts at -0x80.
    HxStr mBitmapPath;               // +0x40
    int mGsHandle;                   // +0x48 Starts at -1, which stands for no residency.
    std::vector<int> mLoadedBitmaps; // +0x4c
};

/**
 * Creator the registered "Tex" class builds through.
 *
 * The default creator allocates 0x58 bytes and constructs a Rnd::Tex. GfxDevice::Init() and the
 * PlayStation 2 texture layer both overwrite the hook with the Rnd::PsTex creator.
 *
 * @ghidraAddress 0x007033a8
 */
extern Tex *(*g_pfnNewTex)(const HxStr &name);

/**
 * Registered class name of Rnd::Tex, the string "Tex".
 *
 * @ghidraAddress 0x007033b0
 */
extern HxStr g_texClassName;

} // namespace Rnd
