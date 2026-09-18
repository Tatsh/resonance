#pragma once

/** The number of bytes each row buffers, which is what the row count divides the zone by. */
constexpr int kSectorCacheRowSize = 0x10000;

/** The space the cache assumes when zones are switched off. */
constexpr int kSectorCacheFallbackSize = 0x80000;

/** Stored in a row's file and sector while the row buffers nothing. */
constexpr int kSectorCacheRowEmpty = -1;

/**
 * One buffered run of a file.
 *
 * Titled after `seccache.cpp`, the file its allocation tag records. A row buffers
 * kSectorCacheRowSize bytes, so a sector here is a 64 KiB chunk rather than a media sector.
 */
struct SectorCacheRow {
    int mFile;      /*!< The buffered file, or kSectorCacheRowEmpty. +0x00 */
    int mSector;    /*!< The buffered chunk index, or kSectorCacheRowEmpty. +0x04 */
    int mUnknown08; /*!< Undetermined, cleared with the rest of the row. +0x08 */
    void *mBuffer;  /*!< The kSectorCacheRowSize buffer. +0x0c */
};

/**
 * Bring up the sector cache.
 *
 * The cache takes the whole of the zone titled `seccache` and divides it into as many
 * kSectorCacheRowSize buffers as fit, so the requested row count applies only when zones are
 * switched off. Every row starts empty. ArkFile::Open() is the caller, on the first mount, and it
 * requests eight rows.
 *
 * @param nRows The row count to use when zones are switched off.
 * @ghidraAddress 0x00554fb8
 */
void InitSectorCache(int nRows);

/**
 * Release every row and the row table.
 *
 * A row buffer taken from a zone goes back through ZoneFree(), and one taken from the allocator
 * goes back through MemFreeTagged().
 *
 * @ghidraAddress 0x005550c8
 */
void ShutdownSectorCache();

/**
 * Drop every row that buffers a file.
 *
 * Each matching row has its file and sector reset to kSectorCacheRowEmpty and its third word
 * cleared. The buffers stay allocated.
 *
 * @param nFile The file whose rows should be dropped.
 * @ghidraAddress 0x00555298
 */
void InvalidateCachedSectors(int nFile);

/**
 * Number of rows the cache was built with.
 *
 * @ghidraAddress 0x008de790
 */
extern int g_nSectorCacheRows;

/**
 * The row table.
 *
 * @ghidraAddress 0x008de794
 */
extern SectorCacheRow *g_pSectorCacheRows;
