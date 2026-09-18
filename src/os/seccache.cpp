#include "os/seccache.h"

#include "os/mem.h"
#include "os/zone.h"

namespace {

// The zone the cache carves its row buffers out of.
constexpr char kSectorCacheZoneName[] = "seccache";

// 0x008de798
int g_bSectorCacheReady;

// 0x00724bf0. Nothing anywhere in the image writes this word. The read below is its only
// reference, so it is permanently zero and the ZoneFree() branch it selects is dead code.
int g_nSectorCacheZone;

} // namespace

int g_nSectorCacheRows;
SectorCacheRow *g_pSectorCacheRows;

void InitSectorCache(int nRows) {
    int nSaved = ZoneGetCurrent();
    int nZone = FindZoneByName(kSectorCacheZoneName);
    ZoneSetCurrent(nZone);
    ZoneReset();

    if (nZone == kNoZone) {
        g_nSectorCacheRows = nRows;
    } else {
        g_nSectorCacheRows = ZoneGetAvail(kSectorCacheFallbackSize) / kSectorCacheRowSize;
    }

    g_pSectorCacheRows = static_cast<SectorCacheRow *>(
        MemAllocTagged(g_nSectorCacheRows * sizeof(SectorCacheRow), __FILE__, __LINE__));
    g_bSectorCacheReady = 1;

    SectorCacheRow *pRow = g_pSectorCacheRows;
    for (int i = 0; i < g_nSectorCacheRows; ++i) {
        pRow->mFile = kSectorCacheRowEmpty;
        pRow->mSector = kSectorCacheRowEmpty;
        pRow->mUnknown08 = 0;
        pRow->mBuffer = ZoneAlloc(kSectorCacheRowSize);
        ++pRow;
    }

    ZoneSetCurrent(nSaved);
}

void ShutdownSectorCache() {
    SectorCacheRow *pRow = g_pSectorCacheRows;
    for (int i = 0; i < g_nSectorCacheRows; ++i) {
        if (g_nSectorCacheZone != kNoZone) {
            // Unreachable. The word is never written, so this branch never runs even though the
            // buffers do come from a zone.
            ZoneFree(pRow->mBuffer);
        } else {
            MemFreeTagged(pRow->mBuffer, __FILE__, __LINE__);
        }
        pRow->mBuffer = nullptr;
        ++pRow;
    }

    MemFreeTagged(g_pSectorCacheRows, __FILE__, __LINE__);
    g_pSectorCacheRows = nullptr;
    g_nSectorCacheRows = 0;
}

void InvalidateCachedSectors(int nFile) {
    for (int i = 0; i < g_nSectorCacheRows; ++i) {
        SectorCacheRow *pRow = &g_pSectorCacheRows[i];
        if (pRow->mFile == nFile) {
            pRow->mFile = kSectorCacheRowEmpty;
            pRow->mSector = kSectorCacheRowEmpty;
            pRow->mUnknown08 = 0;
        }
    }
}
