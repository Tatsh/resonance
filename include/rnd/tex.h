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
 * The vtable has sixteen entries, so the class declares eight virtuals of its own beyond the six
 * of Rnd::Object. Slots 8 through 12 at `0x004e73c8`, `0x004e75a0`, `0x004e75f8`, `0x004e7600`,
 * and `0x004e7608` are small routines that are not yet identified.
 *
 * Recovery is partial. The three configuration words the loader passes to SetBitmapConfig() are
 * not yet identified, so they are recorded by offset.
 *
 * Two routines that look like members are not. `0x004e4bd8` and `0x004e7b70` both take the address
 * of the mBitmapPath member rather than the texture, and `0x004e4d88`, which both of them finish
 * with, takes the same string and lowercases a copy of it. They belong to the art library that
 * owns the bitmap file rather than to this class, which is also where
 * `C:/FREQ/src/rndartt/abitmap.h` and the `ABmpFile` descriptor point. SetBitmapConfig() reaches
 * them to install the path, choosing `0x004e7b70` for a verbatim path and `0x004e4bd8` to prefix
 * the texture directory stored at `0x007033b8`.
 */
class Tex : public Object {
public:
    /**
     * Construct a texture with no bitmap.
     *
     * The mip selector starts at -0x80 and the GS handle at -1, which stands for no residency.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x004e3dc8
     */
    Tex(const HxStr &name);

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
     * Advance the asynchronous mip loads and report whether any level is still outstanding.
     *
     * Each pending level is polled once. A level that has arrived is stored, reported through
     * OnMipLoaded(), and cleared from the pending mask. A level that failed is reported through the
     * failure sink and also cleared, which stops a caller spinning on a read that will never
     * finish. OnAllMipsLoaded() runs once the mask empties.
     *
     * @return True once no level is outstanding, including after a failure.
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
     * Cancel whatever mip reads are still outstanding.
     *
     * @ghidraAddress 0x004e4648
     */
    void CancelPendingMips();

    /**
     * Release every loaded bitmap and cancel the outstanding reads.
     *
     * Vtable slot 13. A bitmap already resident in GS memory belongs to its slot, so only a copy
     * that never arrived there is released, and the release is billed to `rndtex.cpp` line 610.
     *
     * @ghidraAddress 0x004e7aa8
     */
    virtual void FreeLoadedBitmaps();

protected:
    /**
     * Take delivery of one mip level whose read has just finished.
     *
     * Vtable slot 15. Empty in Rnd::Tex. Rnd::PsTex uploads the level to GS memory here. The name
     * is inferred from the position of the call inside PollAsyncMips().
     *
     * @ghidraAddress 0x004e5928
     */
    virtual void OnMipLoaded();

    /**
     * Take delivery of the last outstanding mip level.
     *
     * Vtable slot 14. Empty in Rnd::Tex, and the point at which Rnd::PsTex knows the whole texture
     * is resident. The name is inferred as above.
     *
     * @ghidraAddress 0x004e4598
     */
    virtual void OnAllMipsLoaded();

protected:
    // Every member is protected rather than private, because Rnd::PsTex reads the mip handles, the
    // pending mask, and the bitmap path while it uploads to GS memory. No access from outside the
    // hierarchy is recovered. The order below is the recovered offset order.
    int mUnknown1c;                // +0x1c
    int mUnknown20;                // +0x20
    int mUnknown24;                // +0x24
    int mUnknown28;                // +0x28
    std::vector<int> mMipHandles;  // +0x2c
    unsigned char mPendingMipMask; // +0x38 One bit per mip level still loading.
    int mMipSelect;                // +0x3c Starts at -0x80.
    HxStr mBitmapPath;             // +0x40
    int mGsHandle;                 // +0x48 Starts at -1, which stands for no residency.
    // The element is a bitmap block the art library allocated, opaque to this class.
    std::vector<void *> mLoadedBitmaps; // +0x4c
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
