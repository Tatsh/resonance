#pragma once

#include <vector>

#include "os/hxstr.h"

/** The number of bytes of the archive header the mount copies into the record. */
constexpr int kArkHeaderSize = 0x100;

/** The archive header version the mount accepts. */
constexpr int kArkVersion = 2;

/** The number of bytes the mount point occupies, which is the rest of the record. */
constexpr int kArkMountPointSize = 0x80;

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
    int mOptimizedFlag;                      // +0x11c parsed out of the path after the mount
    void *mOptimizedTable;                   // +0x120
    int mHasOptimizedTable;                  // +0x124
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

/**
 * Empty string substituted for an archive whose own path buffer is null.
 *
 * The name is the one already applied in Ghidra, which agrees with the use here.
 *
 * @ghidraAddress 0x006fbd10
 */
extern const char *g_pszEmpty;
