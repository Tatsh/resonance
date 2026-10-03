#pragma once

#include <libgraph.h>

#include "rndartt/abitmap.h"

/** Bytes in one GS video memory block, the unit every address and size in this unit uses. */
constexpr int kVramBlockBytes = 256;

/** Blocks of GS video memory on the console, 4 MiB divided by the block size. */
constexpr int kVramBlocks = 0x4000;

/** Blocks in one GS page, which is 8 KiB. */
constexpr int kVramPageBlocks = 32;

/** Records in the block table, all of them file scope and linked into one pool at start-up. */
constexpr int kVramTableEntries = 1024;

/** Records in the palette table. */
constexpr int kVramPalEntries = 1024;

/** Palette slots the reserved palette region divides into. */
constexpr int kVramPalSlots = 192;

/** Words of the palette occupancy bitmap, one bit per slot. */
constexpr int kVramPalSlotWords = 6;

/** Blocks one palette occupies, so 1 KiB for a 256 entry 32-bit table. */
constexpr int kVramPalBlocks = 4;

/** Blocks the palette region reserves, which is every slot at its full size. */
constexpr int kVramPalAreaBlocks = kVramPalSlots * kVramPalBlocks;

/** Rotating lock generations, so a block locked for one packet is released two packets later. */
constexpr int kVramLockGenerations = 3;

/** Record lists the table maintains, which is one head and one tail array of this length. */
constexpr int kVramListCount = 3;

/** Records one generation may lock. */
constexpr int kVramLockedPerGeneration = 128;

/**
 * Kind of block a table record describes.
 *
 * The value is a single byte at `+0x04`. Only four of its values appear in the image, and the two
 * that arrive from outside come from `Rnd::PsTex`. The titles of the first three are inferred from
 * the call sites; only kVramBlockKindFree is settled, because Rnd::VRAM::Entry::MakeFree() writes
 * it and Rnd::VRAM::AllocBlock() tests for it when deciding whether a block may be split.
 */
enum VramBlockKind {
    kVramBlockKindNone = 0,    /*!< Written by Rnd::VRAM::Entry::Reset() on a released record. */
    kVramBlockKindTexture = 2, /*!< Passed by Rnd::PsTex::RestoreSurfaces(). */
    kVramBlockKindRenderTarget = 3, /*!< Rounded up to a page by GetBlockAddr(). */
    kVramBlockKindFree = 4          /*!< Unallocated space the allocator may split. */
};

/**
 * Which of the table's three record lists a record belongs to.
 *
 * Rnd::VRAM::UnlinkEntry() takes the value as an index into the head and tail arrays. The three
 * lists therefore share one pair of arrays rather than six separate pointers. Every title is
 * inferred from what the list is walked for.
 */
enum VramList {
    kVramListFree = 0, /*!< Blocks available for allocation, walked best fit. */
    kVramListUsed = 1, /*!< Blocks a surface owns, walked for the least recently used victim. */
    kVramListPool = 2  /*!< Records describing no block, the supply GetAvailEntry() draws from. */
};

/** Slots one word of the palette occupancy bitmap covers. */
constexpr int kVramPalSlotsPerWord = 32;

/** Palettes SwapOutOldest() releases at most in one pass. */
constexpr int kVramPalSwapOutLimit = 16;

/** Buffer width, in 64 texel units, that a palette transfer uses. A palette is never wider. */
constexpr int kVramPalTbw = 1;

/** Texels one unit of a GS transfer buffer width covers. */
constexpr int kGsTexelsPerTbwUnit = 64;

/**
 * GS pixel storage modes this unit distinguishes.
 *
 * Three of these select a page shape other than the default. The remaining modes take the 64 by 32
 * texel page Rnd::VRAM::Entry::ComputeBlockSize() falls back to, and the jump table the routine
 * dispatches through routes PSMCT16S to that fallback rather than to the 64 by 64 page the hardware
 * gives it. The two 32-bit and 24-bit modes appear because g_anGsPixelStorageModes maps bitmap
 * formats onto them.
 */
enum GsPixelStorageMode {
    kGsPsmCt32 = 0, /*!< Thirty-two bits per texel, direct colour. */
    kGsPsmCt24 = 1, /*!< Twenty-four bits per texel, direct colour. */
    kGsPsmCt16 = 2, /*!< Sixteen bits per texel, direct colour. */
    kGsPsmT8 = 19,  /*!< Eight bits per texel, indexed. */
    kGsPsmT4 = 20   /*!< Four bits per texel, indexed. */
};

/** Bytes of the frame counter run EndFrame() clears, from mSwaps through mSpareFrameCounter. */
constexpr int kVramFrameCounterBytes = 28;

/** Texels along each edge of a tile Rnd::VRAM::WipeVram() writes. */
constexpr int kVramWipeTileTexels = 64;

/** Texel rows Rnd::VRAM::WipeVram() walks. */
constexpr int kVramWipeRows = 512;

/** Blocks Rnd::VRAM::WipeVram() advances its destination by for each texel row. */
constexpr int kVramWipeBlocksPerRow = 64;

/** Bytes of zeros Rnd::VRAM::WipeVram() builds, one tile at four bytes per texel. */
constexpr int kVramWipeTileBytes = kVramWipeTileTexels * kVramWipeTileTexels * 4;

/** Multiple of a request that eviction frees before the allocator retries its search. */
constexpr int kVramSwapBudgetMultiple = 4;

/** Bit of a record's lock mask that pins it against eviction until a caller clears it. */
constexpr int kVramLockPinned = 8;

/** Bits of a record's lock mask the rotating generations own. */
constexpr int kVramLockGenerationMask = 7;

/**
 * One entry of the GS video memory palette table.
 *
 * The title is inferred, from the diagnostics "VRAM pal entry should have lockmask %d set" and
 * "No unlocked VRAM pal entries to swap out!". mMemAddr, mpPrev, and mpNext are attested verbatim
 * by "Pal entry in chain has mMemAddr of 0 (mpPrev: %p, mpNext: %p)".
 *
 * A palette occupies kVramPalBlocks blocks inside the region Rnd::VRAM::Init() reserves below the
 * texture pool, and which slots of that region are taken is tracked in a bitmap rather than in a
 * free list. Resident records form one chain, most recently used first, which is the order
 * SwapOutOldest() walks backwards.
 *
 * The image records no access control. Every member and method below is reached from Rnd::VRAM or
 * from Rnd::PsTex. A friend declaration fits the image as well as the public section written here.
 */
class VramPalEntry {
public:
    /**
     * Take a free palette slot, evicting unlocked palettes when every slot is taken.
     *
     * The routine is the one place in the unit whose title is attested by a literal, its own
     * diagnostic "AllocBlock: trying to set vpalIndex: %d, already set".
     *
     * @return First block of the slot.
     * @ghidraAddress NTSC-U/C: 0x00513900
     * @ghidraAddress PAL: 0x00553be8
     */
    static int AllocBlock();

    /**
     * Release up to sixteen unlocked palettes, oldest first.
     *
     * @return Slot index of the last palette released.
     * @ghidraAddress NTSC-U/C: 0x00513780
     * @ghidraAddress PAL: 0x00553a68
     */
    static int SwapOutOldest();

    /**
     * Slot index of the record's blocks within the palette region.
     *
     * @return Index in the range zero to kVramPalSlots.
     * @ghidraAddress NTSC-U/C: 0x005153f0
     * @ghidraAddress PAL: 0x00555720
     */
    int GetSlotIndex() const;

    /**
     * Release every lock on the record.
     *
     * @ghidraAddress NTSC-U/C: 0x005154c8
     * @ghidraAddress PAL: 0x005557f8
     */
    void ClearLockMask();

    /**
     * Mark the record as used this packet and return the GS address of its palette.
     *
     * @return First block of the slot, or zero while the record is not resident.
     * @ghidraAddress NTSC-U/C: 0x00513a40
     * @ghidraAddress PAL: 0x00553d28
     */
    int GetBlockAddr();

    /**
     * Move a palette into the record's slot, allocating the slot first when needed.
     *
     * @param pSource Palette entries to move.
     * @param nWidth Palette width in entries.
     * @param nHeight Palette height in entries.
     * @param nBitsPerPixel Bits per entry. The routine does not read it.
     * @param nPsm GS pixel storage mode.
     * @return First block of the slot.
     * @ghidraAddress NTSC-U/C: 0x00515590
     * @ghidraAddress PAL: 0x005558c0
     */
    int UploadClut(const void *pSource, int nWidth, int nHeight, int nBitsPerPixel, int nPsm);

    /**
     * Release the record, freeing its slot and returning itself to the free list.
     *
     * @ghidraAddress NTSC-U/C: 0x005154d0
     * @ghidraAddress PAL: 0x00555800
     */
    void FreeSelf();

    /** First block of the slot, zero while the record is not resident. Attested. */
    unsigned short mMemAddr;
    /** Generation bits. The literals write it "lockmask". */
    unsigned short mLockMask;
    /** Value of Rnd::VRAM::mFlushCount at the last GetBlockAddr(). */
    int mLastUsed;
    /** Previous record, towards the most recently used end of the chain. Attested. */
    VramPalEntry *mpPrev;
    /** Next record, towards the least recently used end. Attested. */
    VramPalEntry *mpNext;
};

namespace Rnd {

/**
 * Cache of GS video memory, divided into a reserved palette region and a pool of texture blocks.
 *
 * The class is not polymorphic and has no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The demo's PrintStats() has the same instructions as PrintStats()
 * here.
 * The methods AllocBlock() and Clear() appear verbatim in the unit's own diagnostics, as do the
 * members mMemAddr, mpPrev, and mpNext, the index vpalIndex, the word "lockmask", and the phrases
 * "VRAM table entry" and "VRAM pal entry".
 *
 * The table is a single file scope instance. Its records are two separate file scope arrays rather
 * than members. The start-up glue constructs the two arrays element by element and the instance not
 * at all, and Init() addresses both arrays absolutely while addressing its members through the
 * instance pointer.
 *
 * Allocation is best fit over the free list. When no free run is large enough the least recently
 * used unlocked resident block of sufficient size is taken instead, and when no such block exists
 * either, resident blocks are released until four times the request has been freed and the search
 * restarts. Recency is the packet counter mFlushCount rather than a frame counter, because
 * GfxDevice::FlushGifPacket() advances it.
 *
 * Eviction is held off by a rotating lock. Every lookup within a packet adds its record to the
 * current generation's lock list, and AdvanceLockCycle() moves to the next generation and releases
 * everything the generation it arrives at locked. With kVramLockGenerations generations a record
 * stays locked for the two packets after the one that used it.
 *
 * The recovered size is 0x65 bytes, which is a lower bound from the highest member written. No
 * derived class and no allocation pins it from outside, and the record arrays begin at the next
 * four byte boundary above it.
 *
 * The image records no access control. The allocator internals are reached only from inside the
 * unit, but from Entry rather than from the table. A friend declaration fits the image as well as
 * the public section written here.
 */
class VRAM {
public:
    class Entry;

    /**
     * Destroy the table.
     *
     * The body is empty. The start-up glue's shutdown branch calls it for the file scope instance,
     * which is what identifies it as the destructor. GfxDevice::Terminate() also calls it
     * explicitly on Rnd::TheVRAM as its last step.
     *
     * @ghidraAddress NTSC-U/C: 0x005149f0
     * @ghidraAddress PAL: 0x00554d20
     */
    ~VRAM();

    /**
     * Reserve the palette region above the display buffers and link every record into the pool.
     *
     * @ghidraAddress NTSC-U/C: 0x00512850
     * @ghidraAddress PAL: 0x00552b38
     */
    void Init();

    /**
     * Release every allocation and rebuild the pool as one free run covering the texture pool.
     *
     * A record still locked when the table is cleared is reported through
     * "Clear() - resetting lock on entry %d".
     *
     * @param bClearPalettes Non-zero to release resident palettes as well as blocks.
     * @ghidraAddress NTSC-U/C: 0x00512970
     * @ghidraAddress PAL: 0x00552c58
     */
    void Clear(int bClearPalettes);

    /**
     * Start a frame.
     *
     * The body is empty. GfxDevice::BeginFrame() calls it.
     *
     * @ghidraAddress NTSC-U/C: 0x00514cd8
     * @ghidraAddress PAL: 0x00555008
     */
    void BeginFrame();

    /**
     * Release every lock, fold the frame counters into the running totals, and reset them.
     *
     * @ghidraAddress NTSC-U/C: 0x00513190
     * @ghidraAddress PAL: 0x00553478
     */
    void EndFrame();

    /**
     * Move to the next lock generation and release everything the generation it arrives at locked.
     *
     * A record whose lock bit is already clear is reported through
     * "VRAM should have lockmask %d set but instead has %d".
     *
     * @ghidraAddress NTSC-U/C: 0x00513298
     * @ghidraAddress PAL: 0x00553580
     */
    void AdvanceLockCycle();

    /**
     * Report occupancy and the cache counters through the log.
     *
     * No call site survives.
     *
     * @param pszPrefix Text written before each line.
     * @ghidraAddress NTSC-U/C: 0x00513438
     * @ghidraAddress PAL: 0x00553720
     */
    void PrintStats(const char *pszPrefix);

    /**
     * Report the loads the previous frame performed.
     *
     * @param pnLoads Receives the transfers.
     * @param pnBlocks Receives the blocks those transfers moved.
     * @ghidraAddress NTSC-U/C: 0x00514ce0
     * @ghidraAddress PAL: 0x00555010
     */
    void GetLastFrameLoads(int *pnLoads, int *pnBlocks) const;

    /**
     * Take an unused record from the pool.
     *
     * Exhaustion is reported through "Out of VRAM Table Entries!!!" and the routine then returns
     * null, which every caller dereferences without checking.
     *
     * @return The record, or null once the pool is empty.
     * @ghidraAddress NTSC-U/C: 0x005149f8
     * @ghidraAddress PAL: 0x00554d28
     */
    Entry *GetAvailEntry();

    /**
     * Take an unused record from the palette free list.
     *
     * @return The record.
     * @ghidraAddress NTSC-U/C: 0x00515360
     * @ghidraAddress PAL: 0x00555690
     */
    VramPalEntry *AllocPalEntry();

    /**
     * Find blocks for a record, evicting resident blocks when the free list cannot serve it.
     *
     * The title is attested, through the diagnostic of the palette allocator of the same name. On
     * success the record is resident and joins the address ordered chain in place of the run it
     * took, and any remainder of that run becomes a new free record. When no free run is large
     * enough, the least recently used unlocked resident block that is large enough is taken whole
     * before any block is evicted.
     *
     * @param pEntry Record to make resident.
     * @param nBlocks Blocks to find.
     * @ghidraAddress NTSC-U/C: 0x00512e08
     * @ghidraAddress PAL: 0x005530f0
     */
    void AllocBlock(Entry *pEntry, unsigned short nBlocks);

    /**
     * Put a record on the free list and merge it with whichever neighbours are also free.
     *
     * @param pEntry Record to free.
     * @ghidraAddress NTSC-U/C: 0x00512b50
     * @ghidraAddress PAL: 0x00552e38
     */
    void FreeBlock(Entry *pEntry);

    /**
     * Take a record out of the address ordered chain and merge across the gap it leaves behind.
     *
     * No call site survives. The body appears inline inside AllocBlock() and FreeBlock().
     *
     * @param pEntry Record to remove.
     * @ghidraAddress NTSC-U/C: 0x00512cf0
     * @ghidraAddress PAL: 0x00552fd8
     */
    void RemoveFromChain(Entry *pEntry);

    /**
     * Take a record off one of the three lists.
     *
     * @param pEntry Record to unlink.
     * @param nList One of VramList.
     * @ghidraAddress NTSC-U/C: 0x00514a48
     * @ghidraAddress PAL: 0x00554d78
     */
    void UnlinkEntry(Entry *pEntry, int nList);

    /**
     * Return a record to the pool and forget which blocks it described.
     *
     * @param pEntry Record to release.
     * @ghidraAddress NTSC-U/C: 0x00514b60
     * @ghidraAddress PAL: 0x00554e90
     */
    void ReleaseEntry(Entry *pEntry);

    /**
     * Absorb one free run into the free run below it.
     *
     * The image calls the routine nowhere. It is the out of line copy of a body the compiler
     * expanded into AllocBlock(), FreeBlock(), and RemoveFromChain(), and those three use it here.
     *
     * @param pKeep Record that grows.
     * @param pAbsorb Record immediately above it, which is released.
     * @ghidraAddress NTSC-U/C: 0x00514bb8
     * @ghidraAddress PAL: 0x00554ee8
     */
    void MergeBlocks(Entry *pKeep, Entry *pAbsorb);

    /**
     * Fill the whole of video memory with zeros, one 64 by 64 tile at a time.
     *
     * No call site survives, so the receiver is unverified and the routine reads no member. Each
     * tile is reported through "Clearing vram at addr: %d ($%x)".
     *
     * @ghidraAddress NTSC-U/C: 0x00514db8
     * @ghidraAddress PAL: 0x005550e8
     */
    static void WipeVram();

    /**
     * Move a region of video memory back into a bitmap.
     *
     * The region starts at the top left of nMemAddr and has the bitmap's size, with the buffer
     * width rounded up to whole 64-texel units and the storage mode taken from
     * g_anGsPixelStorageModes. The data cache is written back first, and the GS paths are drained
     * before and after the transfer.
     *
     * @param pBitmap Bitmap to fill, which supplies the shape and the destination.
     * @param nMemAddr First block to read.
     * @ghidraAddress NTSC-U/C: 0x00514d00
     * @ghidraAddress PAL: 0x00555030
     */
    void ReadBackBitmap(ABitmap *pBitmap, unsigned short nMemAddr);

    /**
     * Write the display buffer to a numbered file.
     *
     * The zone "temp" is reset and supplies a 16-bit bitmap of the display size, which
     * ReadBackBitmap() fills from block 0. The red and blue channels are swapped without testing
     * g_nSkipColorSwap, and AGfxFile::Write() writes `<pszName>_<n>.bmp`, n being
     * g_nScreendumpIndex before it advances. The previous zone is selected again once the pixels
     * are allocated.
     *
     * @param pszName Stem of the file to write.
     * @ghidraAddress NTSC-U/C: 0x00513528
     * @ghidraAddress PAL: 0x00553810
     */
    void Screendump(const char *pszName);

    /** Blocks released by eviction this frame. */
    int mSwaps;
    /** Transfers into video memory this frame. */
    int mLoads;
    /** Blocks those transfers moved. */
    int mLoadBlocks;
    /** Lookups this frame that found their record not resident. */
    int mMisses;
    /** Merges of adjacent free runs this frame. */
    int mFreeMerges;
    /** Lookups this frame, which is the divisor of the two reported percentages. */
    int mRequests;
    /*!< A seventh frame counter, cleared with the others and never advanced or read. +0x18 */
    int mSpareFrameCounter;
    /** Running total of mSwaps. */
    int mAccumSwaps;
    /** Running total of mLoads. */
    int mAccumLoads;
    /** Running total of mLoadBlocks. */
    int mAccumLoadBlocks;
    /** Running total of mMisses. */
    int mAccumMisses;
    /** Running total of mFreeMerges. */
    int mAccumFreeMerges;
    int mReserved30; /*!< No instruction reads or writes it. +0x30 */
    /** Blocks currently allocated out of the texture pool. */
    int mBlocksInUse;
    /** Packets submitted, advanced by GfxDevice::FlushGifPacket() through AdvanceLockCycle(). */
    int mFlushCount;
    /** Lowest record of the address ordered chain. */
    Entry *mpChainHead;
    /** Highest record of the address ordered chain. */
    Entry *mpChainTail;
    /** First record of each of the three lists, indexed by VramList. */
    Entry *mpListHead[kVramListCount];
    /** Last record of each of the three lists, indexed by VramList. */
    Entry *mpListTail[kVramListCount];
    /** First block of the palette region. */
    int mPaletteBase;
    /** First block above the palette region, where the texture pool begins. */
    int mPaletteEnd;
    /** Generation, from one to kVramLockGenerations, that a lookup locks into. */
    unsigned char mLockGeneration;
};

/**
 * One entry of the GS video memory block table.
 *
 * The class is not polymorphic and has no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The demo's SetUnused() and InitRaw() have the same instructions as
 * Reset() and MakeFree(). Of the members, only mMemAddr, mpPrev, and mpNext are attested by a
 * literal, the first two through the sibling diagnostic
 * "Pal entry in chain has mMemAddr of 0 (mpPrev: %p, mpNext: %p)" and the parallel layout of the
 * two records. Every other title is inferred from use.
 *
 * A record describes a run of blocks at mMemAddr, whether that run is free or owned by a surface.
 * It appears on exactly one of the three lists through mpPrev and mpNext, and separately on the
 * address ordered chain through mpLower and mpUpper. The chain is what lets a freed run merge with
 * the runs on either side of it.
 *
 * The image records no access control. Every member and method below is reached from VRAM or from
 * Rnd::PsTex. A friend declaration fits the image as well as the public section written here.
 */
class VRAM::Entry {
public:
    /**
     * Mark the record as a free run of the given size.
     *
     * @param nMemAddr First block of the run.
     * @param nBlocks Blocks in the run.
     * @ghidraAddress NTSC-U/C: 0x00515148
     * @ghidraAddress PAL: 0x00555478
     */
    void MakeFree(unsigned short nMemAddr, unsigned short nBlocks);

    /**
     * Forget which blocks the record described.
     *
     * @ghidraAddress NTSC-U/C: 0x00514ee8
     * @ghidraAddress PAL: 0x00555218
     */
    void Reset();

    /**
     * Pin the record against eviction, or release the pin.
     *
     * @param bPinned Non-zero to pin.
     * @ghidraAddress NTSC-U/C: 0x00514ef8
     * @ghidraAddress PAL: 0x00555228
     */
    void SetPinned(int bPinned);

    /**
     * Size the record for an image and put it on the used list, without allocating any blocks.
     *
     * The record starts non-resident, with mMemAddr zero, so the first GetBlockAddr() counts a miss
     * and the first UploadImage() allocates.
     *
     * @param nWidth Image width in texels.
     * @param nHeight Image height in texels.
     * @param nBitsPerPixel Bits per texel. The routine does not read it.
     * @param nPsm GS pixel storage mode.
     * @param nKind One of VramBlockKind.
     * @ghidraAddress NTSC-U/C: 0x00514f18
     * @ghidraAddress PAL: 0x00555248
     */
    void SetupSurface(int nWidth, int nHeight, int nBitsPerPixel, int nPsm, int nKind);

    /**
     * Blocks an image of the given shape occupies.
     *
     * The count is whole GS pages, because the allocator works in pages rather than in texels. The
     * page shape follows the storage mode, 128 by 128 texels for four-bit, 128 by 64 for eight-bit,
     * 64 by 64 for sixteen-bit, and 64 by 32 for everything else.
     *
     * @param nWidth Image width in texels.
     * @param nHeight Image height in texels.
     * @param nBitsPerPixel Bits per texel. The routine does not read it.
     * @param nPsm GS pixel storage mode.
     * @return Blocks the image occupies.
     * @ghidraAddress NTSC-U/C: 0x00515058
     * @ghidraAddress PAL: 0x00555388
     */
    int ComputeBlockSize(int nWidth, int nHeight, int nBitsPerPixel, int nPsm) const;

    /**
     * Mark the record as used this packet and return the GS address of its blocks.
     *
     * The first call within a packet adds the record to the current generation's lock list, which
     * protects it from eviction until the generation comes round again. A record with no blocks
     * counts a miss and returns zero, and the caller is expected to upload.
     *
     * @return First block of the run, rounded up to a page for a render target.
     * @ghidraAddress NTSC-U/C: 0x00513690
     * @ghidraAddress PAL: 0x00553978
     */
    int GetBlockAddr();

    /**
     * Move an image into the record's blocks, allocating them first if the record is not resident.
     *
     * @param pSource Texels to move, or null to allocate without transferring.
     * @param nWidth Image width in texels.
     * @param nHeight Image height in texels.
     * @param nBitsPerPixel Bits per texel. The routine does not read it.
     * @param nPsm GS pixel storage mode.
     * @return First block of the run, rounded up to a page for a render target.
     * @ghidraAddress NTSC-U/C: 0x00515160
     * @ghidraAddress PAL: 0x00555490
     */
    int UploadImage(const void *pSource, int nWidth, int nHeight, int nBitsPerPixel, int nPsm);

    /**
     * Move an image into the record's blocks at an offset, without allocating.
     *
     * A mip chain uses it for every level after the first.
     *
     * @param pSource Texels to move.
     * @param nWidth Image width in texels.
     * @param nHeight Image height in texels.
     * @param nBitsPerPixel Bits per texel. The routine does not read it.
     * @param nPsm GS pixel storage mode.
     * @param nBlockOffset Blocks past mMemAddr to write at.
     * @ghidraAddress NTSC-U/C: 0x005152a8
     * @ghidraAddress PAL: 0x005555d8
     */
    void UploadSubImage(const void *pSource,
                        int nWidth,
                        int nHeight,
                        int nBitsPerPixel,
                        int nPsm,
                        int nBlockOffset);

    /**
     * Release the record, returning its blocks to the free list and itself to the pool.
     *
     * @ghidraAddress NTSC-U/C: 0x00514fa8
     * @ghidraAddress PAL: 0x005552d8
     */
    void FreeSelf();

    /** First block of the run, zero while the record is not resident. Attested. */
    unsigned short mMemAddr;
    /** Blocks in the run. */
    unsigned short mSize;
    /** One of VramBlockKind. */
    unsigned char mKind;
    /** Generation bits plus kVramLockPinned. The literals write it "lockmask". */
    unsigned char mLockMask;
    unsigned short mReserved06; /*!< No instruction reads or writes it. +0x06 */
    /** Value of VRAM::mFlushCount at the last GetBlockAddr(). */
    int mLastUsed;
    /** Previous record on whichever of the three lists this record is on. Attested. */
    Entry *mpPrev;
    /** Next record on the same list. Attested. */
    Entry *mpNext;
    /** Record describing the run immediately below this one, or null at the bottom. */
    Entry *mpLower;
    /** Record describing the run immediately above this one, or null at the top. */
    Entry *mpUpper;
};

} // namespace Rnd

/**
 * GS storage mode of each ABitmapFormat, indexed by the format code.
 *
 * The run-length format maps to PSMT8, the form it is decompressed into before upload.
 *
 * @ghidraAddress NTSC-U/C: 0x0070d3d0
 * @ghidraAddress PAL: 0x007512c0
 */
extern const int g_anGsPixelStorageModes[kABitmapFormatCount];

/**
 * Bits per texel of each ABitmapFormat once it reaches GS memory, indexed by the format code.
 *
 * @ghidraAddress NTSC-U/C: 0x0070d3e8
 * @ghidraAddress PAL: 0x007512d8
 */
extern const int g_anBitsPerPixelTable[kABitmapFormatCount];

namespace Rnd {

/**
 * The video memory cache.
 *
 * @ghidraAddress NTSC-U/C: 0x0070d400
 * @ghidraAddress PAL: 0x007512f0
 */
extern VRAM TheVRAM;

} // namespace Rnd

/**
 * Every block table record.
 *
 * @ghidraAddress NTSC-U/C: 0x0070d468
 * @ghidraAddress PAL: 0x00751358
 */
extern Rnd::VRAM::Entry g_vramEntries[kVramTableEntries];

/**
 * Every palette table record.
 *
 * @ghidraAddress NTSC-U/C: 0x00714468
 * @ghidraAddress PAL: 0x00758358
 */
extern VramPalEntry g_vramPalEntries[kVramPalEntries];

/**
 * First unused palette record.
 *
 * @ghidraAddress NTSC-U/C: 0x0086f6c0
 * @ghidraAddress PAL: 0x008b3da0
 */
extern VramPalEntry *g_pVramPalFree;

/**
 * First resident palette record, most recently used first.
 *
 * @ghidraAddress NTSC-U/C: 0x0086f6c4
 * @ghidraAddress PAL: 0x008b3da4
 */
extern VramPalEntry *g_pVramPalUsed;

/**
 * Occupancy of the palette region, one bit per slot.
 *
 * @ghidraAddress NTSC-U/C: 0x008ef098
 * @ghidraAddress PAL: 0x00934098
 */
extern unsigned g_adVramPalSlots[kVramPalSlotWords];

/**
 * Block records each generation has locked.
 *
 * @ghidraAddress NTSC-U/C: 0x008ef5a0
 * @ghidraAddress PAL: 0x009345a0
 */
extern int g_anVramLockCount[kVramLockGenerations];

/**
 * Palette records each generation has locked.
 *
 * @ghidraAddress NTSC-U/C: 0x008ef6f0
 * @ghidraAddress PAL: 0x009346f0
 */
extern int g_anVramPalLockCount[kVramLockGenerations];

/**
 * Block records each generation has locked, in the order they were locked.
 *
 * @ghidraAddress NTSC-U/C: 0x00902380
 * @ghidraAddress PAL: 0x009473a0
 */
extern Rnd::VRAM::Entry *g_apVramLocked[kVramLockGenerations][kVramLockedPerGeneration];

/**
 * Palette records each generation has locked, in the order they were locked.
 *
 * @ghidraAddress NTSC-U/C: 0x008f0140
 * @ghidraAddress PAL: 0x00935140
 */
extern VramPalEntry *g_apVramPalLocked[kVramLockGenerations][kVramLockedPerGeneration];

/**
 * Transfers the previous frame performed.
 *
 * @ghidraAddress NTSC-U/C: 0x0089df80
 * @ghidraAddress PAL: 0x008e2fc0
 */
extern int g_nVramLoadsLastFrame;

/**
 * Blocks the previous frame's transfers moved.
 *
 * @ghidraAddress NTSC-U/C: 0x0089df84
 * @ghidraAddress PAL: 0x008e2fc4
 */
extern int g_nVramLoadBlocksLastFrame;

/**
 * Transfer descriptor Rnd::VRAM::Entry::UploadImage() reuses.
 *
 * @ghidraAddress NTSC-U/C: 0x0089de60
 * @ghidraAddress PAL: 0x008e2ea0
 */
extern sceGsLoadImage g_vramUploadLoadImage;

/**
 * Transfer descriptor Rnd::VRAM::Entry::UploadSubImage() reuses.
 *
 * @ghidraAddress NTSC-U/C: 0x0089dec0
 * @ghidraAddress PAL: 0x008e2f00
 */
extern sceGsLoadImage g_vramUploadSubLoadImage;

/**
 * Transfer descriptor VramPalEntry::UploadClut() reuses.
 *
 * @ghidraAddress NTSC-U/C: 0x0089df20
 * @ghidraAddress PAL: 0x008e2f60
 */
extern sceGsLoadImage g_vramPalLoadImage;

/**
 * Transfer descriptor Rnd::VRAM::WipeVram() reuses.
 *
 * @ghidraAddress NTSC-U/C: 0x0089de00
 * @ghidraAddress PAL: 0x008e2e40
 */
extern sceGsLoadImage g_vramWipeLoadImage;

/**
 * Transfer descriptor Rnd::VRAM::ReadBackBitmap() reuses, and therefore Rnd::VRAM::Screendump().
 *
 * @ghidraAddress NTSC-U/C: 0x0089dd90
 * @ghidraAddress PAL: 0x008e2dd0
 */
extern sceGsStoreImage g_vramReadBackStoreImage;

/**
 * Number the next Rnd::VRAM::Screendump() file takes, advanced after each dump.
 *
 * @ghidraAddress NTSC-U/C: 0x00718468
 * @ghidraAddress PAL: 0x0075c358
 */
extern int g_nScreendumpIndex;
