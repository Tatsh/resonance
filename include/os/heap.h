#pragma once

#include <stdint.h>

/**
 * The number of bytes the header occupies at the front of the block.
 *
 * The value is written as a constant rather than as `sizeof(Heap)` because it is the size on the
 * 32-bit target, which is what every offset in this header records.
 */
constexpr int kHeapHeaderSize = 0x28;

/** The number of bytes of a node that precede its payload. */
constexpr int kHeapNodePrologueSize = 8;

/** The number of bytes a free node occupies, which is all four of its words. */
constexpr int kHeapFreeNodeSize = 0x10;

/** The smallest payload a node may serve. */
constexpr int kHeapMinPayloadSize = 8;

/** Every payload size is rounded up to a multiple of this. */
constexpr int kHeapPayloadAlignment = 8;

/**
 * A remainder smaller than this is not worth splitting off.
 *
 * The threshold is the full 16-byte free node plus the smallest payload, so every free node the
 * heap produces can serve at least one allocation.
 */
constexpr int kHeapSplitThreshold = 0x18;

/** Take the first free node that fits rather than the smallest. This is the default. */
constexpr unsigned kHeapFlagFirstFit = 0x1;

/** Take the smallest free node that fits. */
constexpr unsigned kHeapFlagBestFit = 0x2;

/** Treat exhaustion as fatal rather than reporting a null block. */
constexpr unsigned kHeapFlagFatalWhenFull = 0x4;

/** The heap allocated its own backing block and releases it when destroyed. */
constexpr unsigned kHeapFlagOwnsBlock = 0x8000;

/**
 * One node of a heap, which is the boundary tag in front of a block.
 *
 * Nodes form one doubly linked run in address order through mPrevAndFree and mNext, and the free
 * nodes additionally form one unordered list through mFreePrev and mFreeNext. An allocated node
 * uses only the first two words, so its payload starts at kHeapNodePrologueSize and overlaps the
 * two free-list words. A free node uses all four.
 *
 * The run ends at a tail node whose mNext is null and which is never allocated from. The stats
 * line titles these records nodes, which is where the name comes from.
 */
struct HeapNode {
    uintptr_t mPrevAndFree; /*!< Preceding node, bit 0 set while free. +0x00 */
    HeapNode *mNext;        /*!< The following node in address order, null at the tail. +0x04 */
    HeapNode *mFreePrev;    /*!< The previous free node, valid only while free. +0x08 */
    HeapNode *mFreeNext;    /*!< The next free node, valid only while free. +0x0c */
};

/**
 * Boundary-tagged heap over one contiguous block.
 *
 * Titled after `Heap.cpp`, the file its allocation tag and its out-of-memory report record. The
 * class is not polymorphic and has no RTTI, so it has no vptr. The 40-byte header sits at the
 * front of the block it manages, and the nodes follow it, which is why Create() reports the block
 * pointer as the heap.
 *
 * The one instance in the image is the arena the embedded Python allocates from, and
 * Py_Initialize() is its only builder. That routine selects the zone titled `python`, asks
 * ZoneGetAvail() for the space left in it with a 2 MiB fallback, takes all of it in one
 * ZoneAlloc(), and builds the heap over that block with kHeapFlagFirstFit and
 * kHeapFlagFatalWhenFull, which is why exhaustion reports `Python heap is out of memory!` and
 * stops. The game itself does not allocate through this class. The global `operator new` and its
 * relatives go to the toolchain allocator instead.
 *
 * A heap therefore sits inside a zone rather than beside one. The chain runs from the toolchain
 * allocator, through the tagged block ZoneCreate() takes for the `python` zone, through the single
 * ZoneAlloc() that claims the whole zone, to the nodes this class hands out. No step ever returns
 * an interior pointer to the allocator above it, and the zone check in MemFreeTagged() and
 * MemReallocTagged() is what enforces that, because releasing an interior pointer would corrupt
 * the block that owns it.
 *
 * Every data member is private, because only the methods below touch them.
 *
 * Every field name comes from the two dump lines, `Heap Info: length: %d flags: %d` and
 * `HHeap Stats: nUsedNodes:%d nFreeNodes:%d nCalls:(M:%d R:%d F:%d) nBytes:%d`. The doubled `H` in
 * the second is reproduced from the image.
 */
class Heap {
public:
    /**
     * Build a heap over a block.
     *
     * The header occupies the first 40 bytes and the usable length is nSize less those bytes. A
     * request with neither fit bit set becomes first-fit.
     *
     * A null block is meant to make the heap allocate one for itself and record kHeapFlagOwnsBlock,
     * but the shipped code discards the allocation and then builds the heap at address zero. The
     * one caller always supplies a block, so the defect never fires.
     *
     * @param pBlock The block to build over, or null to allocate one.
     * @param nSize The whole block size in bytes, header included.
     * @param nFlags Any combination of the kHeapFlag values.
     * @return The heap, which is the block itself.
     * @ghidraAddress 0x00552040
     */
    static Heap *Create(void *pBlock, unsigned nSize, unsigned nFlags);

    /**
     * Release the backing block.
     *
     * Performs no work unless the heap allocated the block itself, because a caller-supplied block
     * belongs to the caller. Nothing is cleared first, so the heap must not be used afterwards.
     *
     * @ghidraAddress 0x00552108
     */
    void Destroy();

    /**
     * Take a block.
     *
     * The request is raised to kHeapMinPayloadSize and rounded up to kHeapPayloadAlignment. A node
     * with more than kHeapSplitThreshold bytes to spare is split and the remainder stays free.
     * Exhaustion is fatal when kHeapFlagFatalWhenFull is set and otherwise reports null.
     *
     * @param nSize The payload size in bytes.
     * @param pszFile The requesting file, which only the report uses.
     * @param nLine The requesting line, which only the report uses.
     * @return The payload, or null when no node fits.
     * @ghidraAddress 0x00551780
     */
    void *Alloc(unsigned nSize, const char *pszFile, int nLine);

    /**
     * Give a block back.
     *
     * A null block produces no work. The node merges with a free neighbour on either side, and
     * joins the free list only when neither neighbour was free.
     *
     * @param pBlock The payload to release.
     * @param pszFile The releasing file, which the shipped body ignores.
     * @param nLine The releasing line, which the shipped body ignores.
     * @ghidraAddress 0x00551d28
     */
    void Free(void *pBlock, const char *pszFile, int nLine);

    /**
     * Resize a block.
     *
     * Growing into a free following node, shrinking in place, and splitting off a remainder all
     * happen without moving the payload. Only a grow that no neighbour can satisfy allocates,
     * copies, and releases.
     *
     * @param pBlock The payload to resize.
     * @param nSize The new payload size in bytes.
     * @param pszFile The requesting file, which the shipped body ignores.
     * @param nLine The requesting line, which the shipped body ignores.
     * @return The payload, which differs from pBlock only when the block moved, or null when the
     *         move could not allocate.
     * @ghidraAddress 0x005519d0
     */
    void *Realloc(void *pBlock, unsigned nSize, const char *pszFile, int nLine);

    /**
     * Shorten the whole heap.
     *
     * Only the trailing free node can be surrendered, so the attempt reports zero when the last
     * node is in use and when the new length would cut into it. On success the backing block is
     * reallocated and the tail node is moved down.
     *
     * @param nSize The new usable length in bytes.
     * @return Non-zero when the heap was shortened.
     * @ghidraAddress 0x00552138
     */
    int Shrink(unsigned nSize);

    /**
     * Write the header, the counters, the timings, and every node to a file.
     *
     * Performs no work when the file cannot be opened. The header and the counters also go to the
     * log.
     *
     * @param pszPath The file to write.
     * @ghidraAddress 0x00551ec8
     */
    void DumpToFile(const char *pszPath);

    /**
     * Write the counters to the log and to the memory report.
     *
     * @ghidraAddress 0x00552218
     */
    void DumpStats();

private:
    // Alloc(), Free(), and Realloc() each repeat these three blocks of free-list relinking, and
    // each body appears in full at every one of those sites, which is what makes them
    // reconstructible as members. None is a trivial accessor.
    void UnlinkFreeNode(HeapNode *pNode);
    void PushFreeNode(HeapNode *pNode);
    void ReplaceFreeNode(HeapNode *pFrom, HeapNode *pTo);

    HeapNode *mStart;    // +0x00 the first node, which follows this header
    unsigned mLength;    // +0x04 usable bytes, excluding this header
    HeapNode *mFreeList; // +0x08
    unsigned mFlags;     // +0x0c
    int mUsedNodes;      // +0x10
    int mFreeNodes;      // +0x14
    int mCallsAlloc;     // +0x18
    int mCallsRealloc;   // +0x1c
    int mCallsFree;      // +0x20
    unsigned mBytes;     // +0x24 payload bytes handed out
};

/**
 * The one interpreter heap, built by Py_Initialize() over the whole of the zone titled
 * `python`.
 *
 * @ghidraAddress 0x00723998
 */
extern Heap *g_pPythonHeap;
