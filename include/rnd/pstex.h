#pragma once

#include <vector>

#include "rnd/tex.h"
#include "rndartt/apalette.h"

class ACanvas;
class HxStr;
class VramPalEntry;
class VramTableEntry;

namespace Rnd {

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
 * Small levels of an eight-bit texture share a GS page. RestoreSurfaces() records in
 * mFirstPackedMip the first level whose larger side is 32 pixels or less, and
 * UploadMipAndBuildMipTbp() places every level from there on inside the page of the level before it
 * at the offsets in g_anPackedMipPageOffsets. Levels below that point each have a page.
 *
 * Two routines have their original names from their own diagnostics rather than from inference.
 * RestoreSurfaces() takes its name from the report "ERROR - RestoreSurfaces(%s), mipmap %d has no
 * bm!" at `0x00831690`. The palette comparison RestoreSurfaces() inlines takes its name from the
 * four "CheckPalEqual(%s)" reports from `0x008315a0` onwards, and its out-of-line copy is at
 * `0x0059a908`.
 *
 * Every data member is private. BindToGsSlot() is public because Rnd::PsMat calls it through a
 * Rnd::Tex pointer it has downcast.
 */
class PsTex : public Tex {
public:
    /**
     * Construct a texture with no GS residency.
     *
     * Only the residency vector, the CLUT staging palette, and mPaletteVram are initialised. The
     * four GS register images and mLockedMip start indeterminate, and RestoreSurfaces() fills them
     * once mip 0 has arrived.
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
     * Give the caller the canvas over a mip level to draw into.
     *
     * Vtable slot 9. Waits for the outstanding mip reads, then yields null unless the level is in
     * range and loaded. Bit 1 of nFlags reads the current GS contents back over the loaded bitmap
     * first, making the lock read-modify-write. The level is recorded for UnlockMipBitmap().
     *
     * @param nMip The mip level.
     * @param nUnknown The second parameter. This override does not read it.
     * @param nFlags Bit 1 requests the read-back.
     * @return The canvas to draw into, or null.
     * @ghidraAddress 0x0059aa48
     */
    virtual ACanvas *LockMipBitmap(int nMip, int nUnknown, int nFlags);

    /**
     * Take back the canvas LockMipBitmap() handed out.
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
     * rebuilds the CLUT staging palette, and sets the dirty-CLUT bit. A null palette rebuilds the
     * staging palette from the entries already present.
     *
     * @param pPalette The replacement palette, or null to rebuild from the current entries.
     * @ghidraAddress 0x0059ab78
     */
    virtual void SetPalette(APalette *pPalette);

    /**
     * Pin the video memory block of mip 0 against eviction, or release the pin.
     *
     * Vtable slot 12, empty in Rnd::Tex. Waits for the outstanding mip reads first. The name is
     * inferred.
     *
     * @param bInUse True to pin the block, false to release it.
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
     * rebuilds the CLUT and allocates its slot for a paletted texture, sizes the residency vector
     * to one entry per level, records mGsPsm and mBitsPerPixel from the bitmap format, finds
     * mFirstPackedMip for an eight-bit format, assembles TEX0 and TEX1, and then for every level
     * validates the dimensions against a power of two of at least 8, validates the format and the
     * palette against level 0, records the level's buffer width in MIPTBP1 or MIPTBP2, marks the
     * level dirty, builds its canvas, and allocates its GS block. A level that fails validation is
     * blanked but still made resident. The Rnd::Tex body runs last.
     *
     * @ghidraAddress 0x00597210
     */
    virtual void RestoreSurfaces();

    /**
     * Make the texture current on the GS, with the given texture function.
     *
     * Waits for the outstanding mip reads, then reports false when no mip 0 bitmap or no residency
     * entry exists. Otherwise it uploads the pending levels, merges the texture function into TEX0,
     * refreshes the CLUT base page when a palette slot exists, and refreshes TBP0. A CLUT or a mip
     * 0 block found evicted is uploaded again. TEX0_1 and TEX1_1 are then programmed, every further
     * level is made resident, and MIPTBP1_1 follows when the texture has more than one level and
     * MIPTBP2_1 when it has more than four.
     *
     * The method is not virtual. Rnd::PsMat calls it through a downcast Rnd::Tex pointer.
     *
     * @param nTexFunc The Rnd::Tex::TexFunc, of which the low two bits go into TEX0.
     * @return True once the texture is resident and bound.
     * @ghidraAddress 0x00598000
     */
    bool BindToGsSlot(unsigned nTexFunc);

    /**
     * Make mip 0 the surface the GS draws into.
     *
     * Waits for the outstanding mip reads. A mip 0 block that is not a render target is released
     * and replaced by a new render target record of the bitmap's shape, and a block that is not
     * resident is allocated without a transfer. FRAME_1 then addresses the block with the buffer
     * width and storage mode of TEX0, XYOFFSET_1 centres the surface in the GS coordinate space,
     * ZBUF_1 masks depth writes, and TEST_1 passes every depth test.
     *
     * Rnd::PsCam::DrawSelf() is the one caller. The routine was previously labelled after surface
     * restoration.
     *
     * @ghidraAddress 0x00596d68
     */
    void BindAsRenderTarget();

    /**
     * Upload one mip level into the video memory block of its level.
     *
     * A level of the run-length format is decompressed into the temporary zone buffer and uploaded
     * as PSMT8 at 8 bits per pixel. Every other level is uploaded from its pixels using mGsPsm and
     * mBitsPerPixel.
     *
     * @param nMip The mip level.
     * @return The first block of the level in video memory.
     * @ghidraAddress 0x00597d50
     */
    int UploadBitmapMipToGs(int nMip);

    /**
     * Resolve one mip level's GS block address, uploading the level when needed.
     *
     * A level at or beyond mFirstPackedMip takes its address from the block of the level before
     * that point plus the packed offset for its distance past it, and uploads nothing. Any other
     * level is uploaded unless bSkipIfResident is set and its block is still resident. Uploading
     * the level just below mFirstPackedMip also uploads every packed level into its block. The
     * address then goes into TEX0, MIPTBP1, or MIPTBP2 for levels 0 through 6.
     *
     * @param nMip The mip level.
     * @param bSkipIfResident True to upload only a level whose block was evicted.
     * @return The first block of the level, or zero when the texture has no residency.
     * @ghidraAddress 0x005982a0
     */
    int UploadMipAndBuildMipTbp(int nMip, bool bSkipIfResident);

    /**
     * Upload every level whose dirty bit is set, then clear the mask.
     *
     * Returns at once while mDirtyMips is clear or mip 0 is missing. A dirty CLUT is uploaded again
     * and its block merged into TEX0, and a dirty CLUT with no palette slot reports "Dirty Palette
     * bit, but no pPaletteVram.. rgba %p" instead. A clean CLUT has its block refreshed and is
     * uploaded again only when found evicted.
     *
     * @ghidraAddress 0x00597e38
     */
    void UploadPendingMips();

    /**
     * Install this class as the texture the renderer builds and program the default filtering.
     *
     * Writes NewPsTex() into Rnd::g_pfnNewTex, then programs the MXL, MMAG, and MMIN fields of GS
     * TEX1_1. GfxDevice::Init() is the one caller.
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
    // One mip level's residency, its video memory block and the canvas LockMipBitmap() hands out.
    // FreeGsSurfaces() deletes the canvas through its virtual destructor.
    struct GsMip {
        VramTableEntry *mPage;
        ACanvas *mVramBitmap;
    };

    /**
     * Release the CLUT slot, every GS page, and every canvas.
     *
     * Empties the residency vector without releasing its storage. The destructor and
     * FreeLoadedBitmaps() are the callers.
     *
     * @ghidraAddress 0x00597130
     */
    void FreeGsSurfaces();

    /**
     * Copy the mip 0 palette into the CLUT staging palette.
     *
     * A four-bit level copies the palette straight through with APalette::SetEntries(). Every
     * other format copies all 256 entries in 32-byte groups permuted by g_anClutSwizzleBlocks,
     * which gives the GS CSM1 layout.
     *
     * @ghidraAddress 0x00597c68
     */
    void RebuildClut();

    /**
     * Claim the GS CLUT slot for this texture.
     *
     * Does nothing once a slot exists or while mip 0 is missing. Reports "Got NULL Palette in
     * RestoreSurfaces" when the manager has none to give. RestoreSurfaces() is the one caller.
     *
     * @ghidraAddress 0x0059abe8
     */
    void AllocPaletteVram();

    /**
     * Upload the CLUT staging palette into the palette slot.
     *
     * A four-bit level uploads 16 entries as an 8 by 2 image, and every other format 256 entries as
     * a 16 by 16 image, both in PSMCT32. UploadPendingMips() and BindToGsSlot() open-code the body,
     * and the out-of-line copy has no caller.
     *
     * @return The first block of the slot.
     * @ghidraAddress 0x0059ac68
     */
    int UploadPaletteClut();

    /**
     * Upload one mip level into another level's block at a block offset.
     *
     * The run-length and direct paths match UploadBitmapMipToGs(). UploadMipAndBuildMipTbp()
     * open-codes the body for the packed levels, and the out-of-line copy has no caller.
     *
     * @param pPage The block to upload into.
     * @param nMip The mip level to upload.
     * @param nBlockOffset The blocks past the start of pPage to write at.
     * @ghidraAddress 0x0059acc0
     */
    void UploadBitmapMipToSubImage(VramTableEntry *pPage, int nMip, int nBlockOffset);

    // Declared in recovered offset order. Every member is private, because no access from outside
    // this class is recovered.

    std::vector<GsMip> mGsMips;  // One entry per loaded mip level.
    unsigned long long mTex0;    // GS TEX0_1 image.
    unsigned long long mTex1;    // GS TEX1_1 image.
    unsigned long long mMipTbp1; // GS MIPTBP1_1 image, levels 1 through 3.
    unsigned long long mMipTbp2; // GS MIPTBP2_1 image, levels 4 through 6.
    int mGsPsm;                  // From g_anGsPixelStorageModes.
    int mBitsPerPixel;           // From g_anBitsPerPixelTable.
    // First level that shares the page of the level before it, or 9999 while none does.
    int mFirstPackedMip;
    // One bit per level awaiting upload, with the sign bit standing for a dirty CLUT.
    unsigned mDirtyMips;
    unsigned long long mUnknown98; // +0x98 Inferred from the alignment of mClut.
    APalette mClut;                // CLUT staging palette, in the GS CSM1 order.
    VramPalEntry *mPaletteVram;    // The manager's CLUT slot, or null.
    int mLockedMip;                // Level recorded by LockMipBitmap().
};

/**
 * Allocate and construct a PlayStation 2 texture.
 *
 * This is the creator StaticInit() installs over Rnd::g_pfnNewTex. The body is
 * `return new PsTex(name);`, and the binary bills the 0x4b0-byte allocation to the tag "Rnd::Tex".
 * The gap between `0x0059a7a8` and `0x0059a7d4` is the exception cleanup that releases the block
 * when the constructor throws.
 *
 * @param name The object name.
 * @return The new texture.
 * @ghidraAddress 0x0059a770
 */
Tex *NewPsTex(const HxStr &name);

/**
 * The palette entry a level that fails validation is left with, an opaque 8888 colour.
 *
 * @ghidraAddress 0x0076f360
 */
extern const unsigned int g_dwDefaultClutEntry;

/**
 * Block groups of a 256-entry CLUT in the order the CSM1 layout stores them, {0, 2, 1, 3}.
 *
 * RebuildClut() permutes each run of four 32-byte groups by it.
 *
 * @ghidraAddress 0x0076f368
 */
extern const int g_anClutSwizzleBlocks[4];

/**
 * Block offset of each packed mip level inside the page of the level before mFirstPackedMip.
 *
 * Indexed by the level's distance past mFirstPackedMip. The six values are 16, 20, 21, 22, 23,
 * and 24. The words after them are zero and have no recovered reader, so the length is a lower
 * bound.
 *
 * @ghidraAddress 0x0076f378
 */
extern const int g_anPackedMipPageOffsets[6];

} // namespace Rnd
