#include "os/inflate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "os/log.h"

// The upstream names resolve to the gzip state the loader defines.
#define inbuf gzipInbuf
#define insize gzipInsize
#define inptr gzipInptr
#define outcnt gzipOutcnt
#define window gzipWindow
#define fill_inbuf GzipRefillInputBuffer
#define flush_window GzipFlushWindow

// Tables come from the fixed pool, and releasing one does nothing. inflate() rewinds the pool.
#define malloc(size) HuftAlloc((size) / sizeof(struct huft))
#define free(p) ((void)(p))

#include "gzip-1.2.4/inflate.c"

enum {
    kHuftPoolSize = 2048, // The number of table entries the pool provides.
};

// NTSC-U/C: 0x008ea930, PAL: 0x0092f930
static struct huft huftTable[kHuftPoolSize];

// NTSC-U/C: 0x007c3a90, PAL: 0x00807790
static struct huft *pHuftNext = huftTable;

// NTSC-U/C: 0x007c3a94, PAL: 0x00807794
// The most pool entries one block has used. Only HuftReset() reads the count.
static int highWater = 0;

// NTSC-U/C: 0x0063e1a8, PAL: 0x0067ed38
void HuftReset(void) {
    const int nUsed = (int)(pHuftNext - huftTable);
    if (highWater < nUsed) {
        highWater = nUsed;
    }
    pHuftNext = huftTable;
}

// NTSC-U/C: 0x0063e1e0, PAL: 0x0067ed70
struct huft *HuftAlloc(unsigned nEntries) {
    struct huft *pTable = pHuftNext;
    pHuftNext = pTable + nEntries;
    if (pHuftNext < huftTable + kHuftPoolSize) {
        return pTable;
    }
    LogPrintf("HUFT MEMORY EXCEEDED!!\n");
    return NULL;
}
