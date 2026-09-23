#pragma once

#include <vector>

#include "os/async.h"
#include "os/hxstr.h"
#include "rnd/filepath.h"
#include "rnd/manager.h"
#include "rnd/object.h"

class ACanvas;
class APalette;
namespace Rnd {
class Stream;
}
struct ABitmap;

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
 * mBitmapPath is a Rnd::FilePath, whose routines sit in this unit and take the path's address
 * rather than the texture's. SetBitmapConfig() installs the path through FilePath::Set() for a
 * verbatim path and through FilePath::SetFromRoot() to prefix FilePath::sRoot.
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

    /** @ghidraAddress 0x004e7628 */
    virtual ~Tex();

    // Rnd::Object leaves slots 3 through 7 pointing at the shared pure-virtual handler at
    // `0x005381a8` and gives slot 2 a body of its own at `0x0053e5a8`. Every one of the six below
    // is a distinct `rndtex.cpp` body in the Rnd::Tex table, so the class declares all six and is
    // not abstract. Rnd::PsTex repeats them byte-identically, which is to say it inherits them.

    /** @ghidraAddress 0x004e4738 */
    virtual void DumpText(FailSink &sink);

    /** @ghidraAddress 0x004e4910 */
    virtual void Save(Stream &stream);

    /** @ghidraAddress 0x004e7610 */
    virtual void Replace(Object *pFrom, Object *pTo);

    /** @ghidraAddress 0x004e7618 */
    virtual const HxStr &ClassName() const;

    /** @ghidraAddress 0x004e79f0 */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /** @ghidraAddress 0x004e4a20 */
    virtual void Load(Stream &stream);

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
     * Records the bitmap dimensions and the mip selector, then either clears the current bitmap or
     * begins loading the new path. The GS associations and the mip handle vector are dropped either
     * way.
     *
     * @param nWidth The bitmap width.
     * @param nHeight The bitmap height.
     * @param nBitsPerPixel The bitmap depth.
     * @param path The bitmap path.
     * @param nMipSelect The mip selector, which starts at -0x80.
     * @param nUnknown28 The fourth configuration word.
     * @ghidraAddress 0x004e7908
     */
    void SetBitmapConfig(int nWidth,
                         int nHeight,
                         int nBitsPerPixel,
                         const HxStr &path,
                         int nMipSelect,
                         int nUnknown28);

    /**
     * Advance the asynchronous mip loads and report whether any level is still outstanding.
     *
     * Each pending level is polled once. A level that has arrived is stored, reported through
     * OnMipLoaded(), and cleared from the pending mask. A level that failed is reported through the
     * failure sink and also cleared, which stops a caller spinning on a read that will never
     * finish. RestoreSurfaces() runs once the mask empties.
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
     * Release the loaded bitmaps and start loading from the configured path again.
     *
     * Vtable slot 8. The body is FreeLoadedBitmaps() followed by AllocateBitmapFromStream().
     * Rnd::PsTex's table addresses a byte-identical per-unit copy at `0x0059a8c8`. Rnd::Movie's
     * SetFrameSelf() calls it after SetBitmapConfig(). The name is the analysis program's.
     *
     * @ghidraAddress 0x004e73c8
     */
    virtual void ReloadBitmaps();

    /** Texture function, the GS `TEX0.TFX` field, which decides how a texel meets the vertex. */
    enum TexFunc {
        kTexFuncModulate = 0,  /*!< Texel times vertex colour. */
        kTexFuncDecal = 1,     /*!< Texel alone. */
        kTexFuncHighlight = 2, /*!< Texel plus vertex colour. */
        kTexFuncHighlight2 = 3 /*!< Texel plus vertex colour, alpha from the texel. */
    };

    /**
     * Lock the bitmap of one mip level for direct access.
     *
     * Vtable slot 9. Rnd::Tex returns null without doing anything. Rnd::PsTex records the level so
     * that UnlockMipBitmap() can mark it dirty. The second parameter is read by neither
     * implementation, so its purpose is unrecovered. The name is inferred from the pairing with
     * slot 10 rather than from any string in the image.
     *
     * @param nMip The mip level.
     * @param nUnknown The second parameter, which no recovered implementation reads.
     * @param nFlags Bit 1 requests a read-back from GS memory.
     * @return The canvas over the level, or null when the class has none.
     * @ghidraAddress 0x004e75a0
     */
    virtual ACanvas *LockMipBitmap(int nMip, int nUnknown, int nFlags);

    /**
     * Release the mip level that LockMipBitmap() locked.
     *
     * Vtable slot 10. Empty in Rnd::Tex. The name is inferred as above.
     *
     * @ghidraAddress 0x004e75f8
     */
    virtual void UnlockMipBitmap();

    /**
     * Replace the palette every mip level shares.
     *
     * Vtable slot 11. Empty in Rnd::Tex. The name is inferred as above.
     *
     * @param pPalette The replacement palette.
     * @param nUnknown A second word no recovered implementation reads. Rnd::Movie's palette chunk
     *                 handler passes -1 explicitly at `0x005cf55c`.
     * @ghidraAddress 0x004e7600
     */
    virtual void SetPalette(APalette *pPalette, int nUnknown);

    /**
     * Mark the texture's GS page as in use or free.
     *
     * Vtable slot 12. Empty in Rnd::Tex. The PlayStation 2 override pins the video memory block of
     * mip 0 against eviction, or releases the pin. The name is inferred.
     *
     * @param bInUse Whether the page is in use.
     * @ghidraAddress 0x004e7608
     */
    virtual void SetGsPageInUse(bool bInUse);

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
     * Block until every requested mip level has arrived, pumping the asynchronous reads meanwhile.
     *
     * Rnd::PsTex open-codes the body in BindToGsSlot(), LockMipBitmap(), SetGsPageInUse(), and the
     * routine at `0x00596d68`. The out-of-line copy has no caller.
     *
     * @ghidraAddress 0x004e75a8
     */
    void WaitForMipsLoaded() {
        if (!IsLoadComplete()) {
            while (!PollAsyncMips()) {
                AsyncPumpCompletedRequests();
            }
        }
    }

    /**
     * Rebuild whatever the texture keeps in GS memory.
     *
     * Vtable slot 14. The name is the routine's own: the PlayStation 2 override reports
     * `"ERROR - RestoreSurfaces(%s), mipmap %d has no bm!"`, and the helper it calls reports
     * `"Got NULL Palette in RestoreSurfaces"`.
     *
     * @ghidraAddress 0x004e4598
     */
    virtual void RestoreSurfaces();

    /**
     * Take delivery of one mip level whose read has just finished.
     *
     * Vtable slot 15. The level's block is a bitmap header followed by a palette and the pixels.
     * An indexed format (4 bit, 8 bit, or run length 8 bit) points the bitmap at both. A direct
     * colour format has no palette, and its pixels start where the palette would. Red and blue are
     * then swapped unless g_nSkipColorSwap is set. Flag 0x10 of mUnknown28 runs
     * ABitmap::SetPaletteAlphaFromLowByte(0), or else flag 0x20 runs it with 1, and
     * ABitmap::ApplyColorKey() receives mUnknown28 whole. A level whose width or height is not a
     * power of two is reported to g_failSink. A level above zero whose size is not mWidth and
     * mHeight shifted right by nMip is reported as well. Neither report stops the load. Rnd::PsTex
     * runs this body first and then uploads the level to GS memory. The name is inferred from the
     * position of the call inside PollAsyncMips().
     *
     * @param nMip The level that arrived, which both implementations use to index the loaded
     * bitmaps.
     * @ghidraAddress 0x004e5928
     */
    virtual void OnMipLoaded(int nMip);

protected:
public:
    // RestoreSurfaces() writes these three from the bitmap it is restoring. They are public rather
    // than protected because Rnd::Cam reads the first two from outside the hierarchy, through a
    // Rnd::Tex pointer, in both UpdateTargetAspect() and SetTargetTex(), and Rnd::PsCam does the
    // same in ScreenToPixels(). The image supplies no accessor for either.
    int mWidth;
    int mHeight;
    int mBitsPerPixel;

protected:
    // Every member below is protected rather than private, because Rnd::PsTex reads the mip
    // handles, the pending mask, and the bitmap path while it uploads to GS memory. No access from
    // outside the hierarchy is recovered for any of them. The order is the recovered offset order.
public:
    /**
     * The fourth configuration word SetBitmapConfig() records. Public because Rnd::Movie's
     * SetFrameSelf() reads it to pass it back unchanged. +0x28
     */
    int mUnknown28;

protected:
    std::vector<int> mMipHandles;  // +0x2c
    unsigned char mPendingMipMask; // +0x38 One bit per mip level still loading.

public:
    /**
     * Mip selector SetBitmapConfig() records, starting at -0x80. Public because Rnd::Movie's
     * SetFrameSelf() reads it to pass it back unchanged. +0x3c
     */
    int mMipSelect;

protected:
    FilePath mBitmapPath; // +0x40
    int mGsHandle;        // +0x48 Starts at -1, which stands for no residency.
    std::vector<ABitmap *> mLoadedBitmaps;
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

/**
 * Allocate and construct a texture, the base creator of the "Tex" class.
 *
 * @param name The object name.
 * @return The new texture.
 * @ghidraAddress 0x004e77f0
 */
Tex *NewTex(const HxStr &name);

/**
 * Build a texture for the registered "Tex" class by calling through g_pfnNewTex.
 *
 * @param name The object name.
 * @return The new texture, as its Rnd::Object subobject.
 * @ghidraAddress 0x004e7770
 */
Object *CreateRegisteredTex(const HxStr &name);

/**
 * Point g_pfnNewTex at NewTex() and register the "Tex" class with Rnd::Manager.
 *
 * Neither out-of-line copy has a caller. The second, at `0x0059a518`, lies in the Rnd::PsTex unit,
 * and GfxDevice::Terminate() expands the body.
 *
 * @ghidraAddress 0x004e7408
 */
inline void RegisterTexClass() {
    g_pfnNewTex = NewTex;
    g_manager.RegisterClass(g_texClassName, CreateRegisteredTex);
}

} // namespace Rnd
