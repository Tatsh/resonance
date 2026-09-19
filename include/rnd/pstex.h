#pragma once

#include <vector>

#include "rnd/tex.h"

class HxStr;

namespace Rnd {

/**
 * Entries in the GS CLUT staging buffer of a PlayStation 2 texture.
 *
 * A 256-entry palette is the largest the GS addresses through CSM1, and the buffer is sized for
 * one regardless of how many entries the bitmap supplies.
 */
constexpr int kPsTexClutEntryCount = 256;

/**
 * PlayStation 2 texture, owner of the GS residency of its mip levels.
 *
 * `Q23Rnd5PsTex` in the RTTI descriptor at `0x008efec0`, with Rnd::Tex as its one public base at
 * offset 0. The vtable is at `0x008318c0` and has the same sixteen entries as the Rnd::Tex table.
 * Slot 0 is the compiler-generated type_info accessor at `0x0059a4c8`, slot 1 the destructor, slots
 * 2 through 7 the Rnd::Object virtuals, and slots 8 through 15 the eight Rnd::Tex declares. This
 * class overrides slots 9 through 15 and supplies its own destructor.
 *
 * Slot 8 is a special case. The Rnd::PsTex table points at `0x0059a8c8` where the Rnd::Tex table
 * points at `0x004e73c8`, and the two bodies are identical. Both invoke the slot 13 virtual and
 * then Rnd::Tex::AllocateBitmapFromStream(). The second copy exists because the member is defined
 * inline in the class body, and each translation unit that emits a vtable emits its own copy. Slot
 * 8 is therefore not an override of this class and is not declared here.
 *
 * A texture becomes a Rnd::PsTex through the creator hook. StaticInit() installs NewPsTex() over
 * Rnd::g_pfnNewTex, and GfxDevice::Init() runs StaticInit() at `0x0049af50`. Every texture the
 * renderer loads from a file on this target is therefore a Rnd::PsTex.
 *
 * The object is 0x4b0 bytes and the subclass occupies `+0x58` through `+0x4af`. Four of the GS
 * register images the class assembles are 64-bit, and RestoreSurfaces() builds them from the mip 0
 * bitmap: TEX0 receives TBP0, TBW, PSM, TW, TH, TCC, TFX, CBP, and CLD, TEX1 receives MXL, MMAG,
 * MMIN, and the LOD bias K from Rnd::Tex::mMipSelect, and MIPTBP1 and MIPTBP2 receive the base page
 * and the buffer width of levels 1 through 6. BindToGsSlot() programs all four through
 * GfxDevice::SetGsReg().
 *
 * Small levels share a GS page. RestoreSurfaces() records in mFirstPackedMip the first level whose
 * larger side is 32 pixels or less, and UploadMipAndBuildMipTbp() places every level from there on
 * inside the page of the level before it at the offsets in `g_anPackedMipPageOffsets`
 * (`0x0076f378`). Levels below that point each own a page.
 *
 * Two routines have their original names from their own diagnostics rather than from inference.
 * RestoreSurfaces() takes its name from the report "ERROR - RestoreSurfaces(%s), mipmap %d has no
 * bm!" at `0x00831690`. The palette comparison RestoreSurfaces() inlines takes its name from the
 * four "CheckPalEqual(%s)" reports from `0x008315a0` onwards, and its out-of-line copy is at
 * `0x0059a908`.
 *
 * Recovery is partial in two respects. First, the art library that owns the mip bitmaps and their
 * palettes has no header in this tree yet, and neither does the PlayStation 2 texture VRAM manager
 * whose singleton is at `0x0070d400`. The two pointers of GsMip and the palette slot therefore stay
 * opaque, and every routine that reads a bitmap field remains declared rather than written.
 * Second, mUnknown4a0 and mUnknown4a4 have no writer in the recovered set. The constructor clears
 * both and the destructor releases mUnknown4a0, and nothing else touches either. That release is
 * therefore dead code on this build unless a writer exists outside the routines recovered so far.
 *
 * Three Rnd::Tex declarations this class overrides disagree with `rnd/tex.h` as it stands. Slot 14
 * appears there as OnAllMipsLoaded(), and the string at `0x00831690` establishes the real name as
 * RestoreSurfaces(). Slot 15 appears there as OnMipLoaded() with no parameter, and both
 * `0x004e5928` and `0x00596fd8` read a mip index out of the second argument register. Slots 9
 * through 12 are not declared there at all. Until those three are corrected the members below
 * introduce new virtuals instead of overriding the base ones.
 */
class PsTex : public Tex {
public:
    /**
     * Construct a texture with no GS residency.
     *
     * Only the residency vector, mUnknown4a0, mUnknown4a4, and mPaletteVram are initialised. The
     * four GS register images, the CLUT staging buffer, and mLockedMip start indeterminate, and
     * RestoreSurfaces() fills them once mip 0 has arrived.
     *
     * @param name The object name, passed to the Rnd::Object constructor.
     * @ghidraAddress 0x00596f80
     */
    PsTex(const HxStr &name);

    /**
     * Release the GS residency and the texture.
     *
     * Vtable slot 1. The Rnd::Tex destructor body is inlined into this one in the shipped build.
     *
     * @ghidraAddress 0x0059a558
     */
    virtual ~PsTex();

    /**
     * Give the caller a mip level's bitmap to draw into.
     *
     * Vtable slot 9, and the Rnd::Tex body returns null. Waits for the outstanding mip reads, then
     * yields null unless the level is in range and loaded. Bit 1 of nFlags reads the current GS
     * contents back over the loaded bitmap first, making the lock read-modify-write. The level is
     * recorded for UnlockMipBitmap().
     *
     * The return type is the art library's bitmap class, and that class has no header in this tree
     * yet. The name is inferred from the pairing with slot 10; the image supplies no string for it.
     *
     * @param nMip The mip level.
     * @param nUnknown The second parameter. This override does not read it.
     * @param nFlags Bit 1 requests the read-back.
     * @return The bitmap to draw into, or null.
     * @ghidraAddress 0x0059aa48
     */
    virtual void *LockMipBitmap(int nMip, int nUnknown, int nFlags);

    /**
     * Take back the bitmap LockMipBitmap() handed out.
     *
     * Vtable slot 10, empty in Rnd::Tex. Marks the recorded level dirty. The next bind uploads
     * that level again.
     *
     * @ghidraAddress 0x0059ab30
     */
    virtual void UnlockMipBitmap();

    /**
     * Replace the palette of the texture.
     *
     * Vtable slot 11, empty in Rnd::Tex. Copies the incoming entries over the mip 0 palette,
     * rebuilds the CLUT staging buffer, and sets the dirty-CLUT bit. A null palette rebuilds the
     * buffer from the entries already present.
     *
     * The parameter type is the art library's APalette, identified by the tag at `0x00831560`.
     *
     * @param pPalette The replacement palette, or null to rebuild from the current entries.
     * @ghidraAddress 0x0059ab78
     */
    virtual void SetPalette(APalette *pPalette);

    /**
     * Mark the page of mip 0 as in use, or release that mark.
     *
     * Vtable slot 12, empty in Rnd::Tex. Waits for the outstanding mip reads, then sets or clears
     * bit 3 of the usage byte of the GS page through the VRAM manager. The name is inferred from
     * that single effect, and the meaning of bit 3 is not recovered.
     *
     * @param bInUse True to set the mark, false to clear it.
     * @ghidraAddress 0x0059a818
     */
    virtual void SetGsPageInUse(bool bInUse);

    /**
     * Release every loaded bitmap and every GS surface.
     *
     * Vtable slot 13.
     *
     * @ghidraAddress 0x0059a7e8
     */
    virtual void FreeLoadedBitmaps();

    /**
     * Rebuild the GS state of the whole texture from the loaded bitmaps.
     *
     * Vtable slot 14. Falls back to the Rnd::Tex body while mip 0 has not arrived. Otherwise it
     * sizes the residency vector to one entry per level, records mGsPsm and mBitsPerPixel from the
     * bitmap format, finds mFirstPackedMip, assembles TEX0 and TEX1, and then for every level
     * validates the dimensions against a power of two of at least 8, validates the format and the
     * palette against level 0, claims the GS page, marks the level dirty, and records the level's
     * buffer width in MIPTBP1 or MIPTBP2.
     *
     * @ghidraAddress 0x00597210
     */
    virtual void RestoreSurfaces();

    /**
     * Bind the texture to a sampler slot.
     *
     * Waits for the outstanding mip reads, then reports false when no mip 0 bitmap or no residency
     * entry exists. Otherwise it flushes the pending uploads, merges the low two bits of nSlot into
     * the TEX0 TFX field, refreshes the CLUT base page and uploads the CLUT when a palette slot
     * exists, refreshes TBP0, and programs TEX0 and TEX1. MIPTBP1 follows when the texture has more
     * than one level and MIPTBP2 when it has more than four.
     *
     * @param nSlot The sampler slot, of which the low two bits select the GS sampler.
     * @return True once the texture is resident and bound.
     * @ghidraAddress 0x00598000
     */
    bool BindToGsSlot(unsigned nSlot);

    /**
     * Upload one mip level to its GS page.
     *
     * A level of the run-length format is decompressed into a temporary zone buffer and uploaded as
     * PSMT8 at 8 bits per pixel. Every other level is uploaded from its own pixels using mGsPsm and
     * mBitsPerPixel.
     *
     * @param nMip The mip level.
     * @ghidraAddress 0x00597d50
     */
    void UploadBitmapMipToGs(int nMip);

    /**
     * Resolve one mip level's GS page address and record it in the register images.
     *
     * A level at or beyond mFirstPackedMip takes its address from the page of the level before that
     * point plus the packed offset for its distance past it, and every packed level is uploaded
     * into the shared page. A level below that point has its own page registered, and an unresident
     * page is uploaded. The address then goes into TEX0, MIPTBP1, or MIPTBP2 for that level.
     *
     * @param nMip The mip level.
     * @param bForceUpload True to register the page even when the level is already resident.
     * @return The GS page address of the level.
     * @ghidraAddress 0x005982a0
     */
    unsigned UploadMipAndBuildMipTbp(unsigned nMip, bool bForceUpload);

    /**
     * Upload every level whose dirty bit is set, then clear the mask.
     *
     * Returns at once while mDirtyMips is clear. A dirty CLUT reloads the palette slot and merges
     * the new base page into TEX0, and a dirty CLUT with no palette slot reports "Dirty Palette
     * bit, but no pPaletteVram.. rgba %p" instead.
     *
     * @ghidraAddress 0x00597e38
     */
    void UploadPendingMips();

    /**
     * Install this class as the texture the renderer builds and program the default filtering.
     *
     * Writes NewPsTex() into Rnd::g_pfnNewTex, then programs GS TEX1_1 with a linear magnification
     * filter and a nearest minification filter. GfxDevice::Init() is the one caller.
     *
     * @ghidraAddress 0x0059a888
     */
    static void StaticInit();

protected:
    /**
     * Take delivery of one mip level whose read has just finished.
     *
     * Vtable slot 15. Runs the Rnd::Tex body, then halves the alpha of the level, converting 0 to
     * 255 source alpha into the GS range of 0 to 128. A paletted level is converted in its palette
     * and a 32-bit level texel by texel. A level of any other unpaletted format is not converted.
     *
     * @param nMip The mip level that arrived.
     * @ghidraAddress 0x00596fd8
     */
    virtual void OnMipLoaded(int nMip);

private:
    // One mip level's residency in GS memory. Both pointers address classes with no header in this
    // tree yet, the VRAM manager's page record and the art library's bitmap. They therefore stay
    // opaque here rather than being given a type the image does not support.
    struct GsMip {
        void *mPage;       // +0x00
        void *mVramBitmap; // +0x04
    };

    /**
     * Release the CLUT slot, every GS page, and every VRAM-backed bitmap.
     *
     * Empties the residency vector without releasing its storage. The destructor and
     * FreeLoadedBitmaps() are the callers.
     *
     * @ghidraAddress 0x00597130
     */
    void FreeGsSurfaces();

    /**
     * Copy the mip 0 palette into the CLUT staging buffer.
     *
     * A 16-entry palette is copied straight through. A 256-entry palette is copied in 32-byte
     * groups permuted by `g_anClutSwizzleBlocks` (`0x0076f368`). The result is the GS CSM1 layout.
     *
     * @ghidraAddress 0x00597c68
     */
    void RebuildClut();

    /**
     * Claim the GS CLUT slot for this texture.
     *
     * Does nothing once a slot exists. Reports "Got NULL Palette in RestoreSurfaces" when the
     * manager has none to give. RestoreSurfaces() is the one caller.
     *
     * @ghidraAddress 0x0059abe8
     */
    void AllocPaletteVram();

    // Declared in recovered offset order. Every member is private, because no access from outside
    // this class is recovered.

    std::vector<GsMip> mGsMips;  // +0x58 One entry per loaded mip level.
    unsigned long long mTex0;    // +0x68 GS TEX0_1 image.
    unsigned long long mTex1;    // +0x70 GS TEX1_1 image.
    unsigned long long mMipTbp1; // +0x78 GS MIPTBP1_1 image, levels 1 through 3.
    unsigned long long mMipTbp2; // +0x80 GS MIPTBP2_1 image, levels 4 through 6.
    int mGsPsm;                  // +0x88 From g_anGsPixelStorageModes.
    int mBitsPerPixel;           // +0x8c From g_anBitsPerPixelTable.
    // First level that shares the page of the level before it, or 9999 while none does.
    int mFirstPackedMip; // +0x90
    // One bit per level awaiting upload, with the sign bit standing for a dirty CLUT.
    unsigned mDirtyMips;                  // +0x94
    unsigned long long mUnknown98;        // +0x98 Inferred from the alignment of mClut.
    unsigned mClut[kPsTexClutEntryCount]; // +0xa0
    void *mUnknown4a0;                    // +0x4a0 Released by the destructor, never written.
    int mUnknown4a4;                      // +0x4a4
    void *mPaletteVram;                   // +0x4a8 The manager's CLUT slot, or null.
    int mLockedMip;                       // +0x4ac Level recorded by LockMipBitmap().
};

/**
 * Allocate and construct a PlayStation 2 texture.
 *
 * This is the creator StaticInit() installs over Rnd::g_pfnNewTex. The body is
 * `return new PsTex(name);`, and the binary bills the 0x4b0-byte allocation to the tag "Rnd::Tex".
 * The gap between `0x0059a7a8` and `0x0059a7d4` is the exception cleanup that releases the block
 * when the constructor throws.
 *
 * The body is not written yet, because Rnd::Tex declares none of the Rnd::Object virtuals in
 * vtable slots 2 through 7 and so remains abstract in this tree.
 *
 * @param name The object name.
 * @return The new texture.
 * @ghidraAddress 0x0059a770
 */
Tex *NewPsTex(const HxStr &name);

} // namespace Rnd
