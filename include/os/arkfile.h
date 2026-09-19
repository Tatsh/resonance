#pragma once

#include <vector>

#include "os/hxstr.h"

/** The number of bytes of the archive header the mount copies into the record. */
constexpr int kArkHeaderSize = 0x100;

/** The archive header version the mount accepts. */
constexpr int kArkVersion = 2;

/** The number of bytes the mount point occupies, which is the rest of the record. */
constexpr int kArkMountPointSize = 0x80;

/** Rows the sector cache is asked for on the first mount. */
constexpr int kArkSectorCacheRows = 8;

/** The number of bytes of the device path Open() builds on its stack. */
constexpr int kArkDevicePathSize = 0x100;

/** Directory the archives live in on the disc. */
constexpr char kArkDiscRoot[] = "cdrom0:\\ARK";

/** Prefix that reads an archive over the host link instead. */
constexpr char kArkHostRoot[] = "host0:";

/** Path component of the archive header that fixes the mount point. */
constexpr char kArkRunComponent[] = "run";

/**
 * One entry of the archive's directory table.
 *
 * Only the name offset has been recovered. Every entry's name offset is rebased at mount time from
 * an archive-relative offset to an offset inside the name pool.
 */
struct ArkDirEntry {
    int mUnknown00;  /*!< Undetermined. +0x00 */
    int mNameOffset; /*!< Offset of the entry name inside the name pool. +0x04 */
    int mUnknown08;  /*!< Undetermined. +0x08 */
    int mUnknown0c;  /*!< Undetermined. +0x0c */
    int mStoredSize; /*!< Bytes as stored, which a read position counts against. +0x10 */
    int mSize;       /*!< Bytes after decompression. +0x14 */
};

/**
 * One entry of the archive's file table.
 *
 * Only the name offset has been recovered, and it is rebased at mount time in the same way as a
 * directory entry.
 */
struct ArkFileEntry {
    int mUnknown00;  /*!< Undetermined. +0x00 */
    int mNameOffset; /*!< Offset of the entry name inside the name pool. +0x04 */
};

/**
 * One open stream on a mounted archive.
 *
 * The record is 20 bytes, and only the two fields ArkFile::Close() reads have been recovered. A
 * handle with bit 0x4000 set identifies a stream inside an archive rather than a loose file.
 */
struct ArkStream {
    int mUnknown00; /*!< Undetermined. +0x00 */
    int mUnknown04; /*!< Undetermined. +0x04 */
    int mFile;      /*!< The archive's file, which is what ties a stream to its archive. +0x08 */
    int mHandle;    /*!< The stream handle, with bit 0x4000 cleared. +0x0c */
    ArkDirEntry *mEntry; /*!< The directory entry this stream reads. +0x10 */
};

/**
 * Mounted ark archive.
 *
 * An ark stores the game's data files in one seekable blob, so the streaming layer can read a file
 * without a directory lookup per file. This class is not polymorphic and has no RTTI, so it has no
 * vptr. Its name comes from the `ArkFile.cpp` allocation tag. The record is 424 bytes and begins
 * with an embedded HxStr, which is why the mount assigns a path through the record pointer itself.
 *
 * A mount copies the whole 256-byte header into the record, then allocates one block for the
 * directory table, the file table, and the name pool together, and rebases every name offset into
 * that block. The header's own path is lowercased in place and must include a `run` component,
 * which is what fixes the archive's mount point.
 *
 * Every data member is private. Only the two static entry points and the destructor touch them,
 * and access from another instance through g_apMountedArks proves nothing, because private access
 * permits it.
 */
class ArkFile {
public:
    /**
     * Mount an ark archive by path.
     *
     * @param pszPath The archive path, relative to the data root.
     * @return Non-zero on success.
     * @ghidraAddress 0x00559858
     */
    static int Open(const char *pszPath);

    /**
     * Construct an unmounted archive.
     *
     * Only the path and the four table pointers are cleared. mFile is deliberately not, which the
     * one unreachable branch of Open() depends on.
     */
    ArkFile();

    /**
     * Unmount a previously mounted archive.
     *
     * The unmount is refused, and reports zero, when the path is not mounted and when a stream is
     * still open on the archive's file. In the second case the offending stream record is erased
     * first.
     *
     * @param pszPath The same path that was passed to Open().
     * @return Non-zero when the archive was unmounted.
     * @ghidraAddress 0x00559f70
     */
    static int Close(const char *pszPath);

    /**
     * Release the tables the mount allocated.
     *
     * The destructor is inlined into Close(), which is where its line number comes from.
     *
     * @ghidraAddress 0x00559f70
     */
    ~ArkFile();

private:
    HxStr mPath;                             // +0x000
    int mFile;                               // +0x008 negative when the open failed
    int mHeaderUnknown0c;                    // +0x00c start of the copied header
    int mVersion;                            // +0x010
    int mTableOffset;                        // +0x014 archive offset of the directory table
    int mDirCount;                           // +0x018
    int mFileTableOffset;                    // +0x01c
    int mFileCount;                          // +0x020
    int mNameOffset;                         // +0x024 archive offset of the name pool
    int mHeaderUnknown28;                    // +0x028
    int mTableEnd;                           // +0x02c archive offset one past the name pool
    int mHeaderUnknown30;                    // +0x030
    int mOptimized;                          // +0x034 set when the optimized block is present
    int mOptimizedOffset;                    // +0x038
    int mOptimizedCount;                     // +0x03c entries, each two bytes
    int mHeaderUnknown40;                    // +0x040
    int mHeaderUnknown44;                    // +0x044
    int mHeaderUnknown48;                    // +0x048
    char mHeaderPath[kArkHeaderSize - 0x40]; // +0x04c lowercased in place by the mount
    void *mTables;                           // +0x10c the one block the three tables live in
    ArkDirEntry *mDirEntries;                // +0x110 mTables
    ArkFileEntry *mFileEntries;              // +0x114 mTables plus the file table's offset
    char *mNames;                            // +0x118 mTables plus the name pool's offset
    int mDiscLsn;                            // +0x11c the archive's start sector on the disc
    void *mOptimizedTable;                   // +0x120
    int mOptimizedCursor;                    // +0x124 slot the next lookup tries first, or -1
    char mMountPoint[kArkMountPointSize];    // +0x128 the header path after its `run` component
};

/**
 * Erase a stream record by handle.
 *
 * @param nHandle The stream handle.
 * @return Zero when the record was erased, or -1 when no record has that handle.
 * @ghidraAddress 0x0055a1a0
 */
int EraseArkStream(int nHandle);

/**
 * Find the record of an open stream.
 *
 * kFileHandleArkStream is masked off the handle before the search. A caller may therefore pass the
 * handle with the bit or without it.
 *
 * @param nHandle The stream handle.
 * @return The record, or null when no record has that handle.
 * @ghidraAddress 0x0055c158
 */
ArkStream *FindOpenArkStream(int nHandle);

/**
 * Report the directory entry a stream reads.
 *
 * The entry is what gives a caller the stream's stored and inflated sizes without a directory
 * search of its own. The search walks the stream vector here rather than through
 * FindOpenArkStream(), and it masks kFileHandleArkStream off the handle in the same way.
 *
 * @param nHandle The stream handle, with the bit or without it.
 * @return The entry, or null when no record has that handle.
 * @ghidraAddress 0x0055be80
 */
ArkDirEntry *GetArkStreamDirEntry(int nHandle);

/**
 * Report the archive a stream reads from.
 *
 * @param nHandle The stream handle.
 * @return The archive's file, or -1 when no record has that handle.
 * @ghidraAddress 0x0055c000
 */
int GetArkStreamArkId(int nHandle);

/**
 * Report the disc sector a mounted archive starts at.
 *
 * A file that is not a mounted archive is fatal, and the report lists every mounted archive's file
 * before stopping.
 *
 * @param nFile The archive's file.
 * @return The archive's start sector.
 * @ghidraAddress 0x0055a590
 */
int ArkfileGetBaseSector(int nFile);

/**
 * Map a chunk of an archive to the chunk position the disc stores it at.
 *
 * An archive built without the optimized block stores its chunks in order, and the argument is
 * returned unchanged. An optimized archive stores them in the order the game reads them, and its
 * optimized block is an array of two-byte logical chunk indices whose slot number is the position
 * on the disc. The lookup is therefore a search of that array for the logical index, returning the
 * slot it was found in.
 *
 * ArkFile::mOptimizedCursor makes the common case constant time. It records the slot after the one
 * the last lookup resolved. That is the slot a sequential read wants next. A miss on the guess
 * reports through the log, abandons the cursor, and falls back to searching the whole array. A
 * chunk that appears in no slot is fatal.
 *
 * A file that is not a mounted archive is reported through the log, and the argument is returned
 * unchanged.
 *
 * @param nFile The archive's file.
 * @param nSector The logical chunk index.
 * @return The chunk position on the disc.
 * @ghidraAddress 0x0055a410
 */
int ArkfileLogicalToPhysicalSector(int nFile, int nSector);

/**
 * Mount the archives the game needs for the whole session.
 *
 * Performs no work when ark archives are not in use. On failure the offending path is reported
 * through the failure sink.
 *
 * @return Non-zero when every archive was mounted.
 * @ghidraAddress 0x004dfb20
 */
int InitArk();

/**
 * Every archive mounted right now.
 *
 * @ghidraAddress 0x00725e90
 */
extern std::vector<ArkFile *> g_apMountedArks;

/**
 * Every stream open on a mounted archive.
 *
 * @ghidraAddress 0x00725ea0
 */
extern std::vector<ArkStream> g_aArkStreams;
