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

/** Bytes of the lowercased path copy OpenArkObject::FindFileEntryByPath() builds on its stack. */
constexpr int kArkPathBufferSize = 0x80;

/** Bytes every caller of OpenArkObject::FindFileEntryByPath() reserves for the file name. */
constexpr int kArkNameBufferSize = 0x80;

/** Bytes every caller of OpenArkObject::FindFileEntryByPath() reserves for the relative path. */
constexpr int kArkRelPathBufferSize = 0x100;

/**
 * Archive index that asks OpenArkObject::FindFileEntryByPath() to search every mounted archive.
 *
 * OpenArkObject::MapPathToArkIndex() never reports it, and the search is therefore unreachable.
 */
constexpr int kArkIndexAny = 100;

/**
 * One entry of the archive's file table.
 *
 * The member titles come from the labels OpenArkObject::PrintFiles() writes: "nameHash", "flags",
 * "nameOffset", "relPathIndex", "sectorOffset", "sector", and "length". The dump does not write
 * mSize, and its title comes from the one reader, the asynchronous loader, which takes it as the
 * inflated size. The name offset is rebased at mount time from an archive-relative offset to an
 * offset inside the string table.
 */
struct ArkFileEntry {
    unsigned short mNameHash;     /*!< HashArkString() of the file name. */
    unsigned short mFlags;        /*!< Flags whose meaning is unrecovered. */
    int mNameOffset;              /*!< Offset of the file name inside the string table. */
    short mRelPathIndex;          /*!< Index of the directory in the relative path table. */
    unsigned short mSectorOffset; /*!< Byte offset of the file inside its first sector. */
    int mSector;                  /*!< Sector the file starts in. */
    int mLength;                  /*!< Bytes as stored, which a read position counts against. */
    int mSize;                    /*!< Bytes after decompression. */
};

/**
 * One entry of the archive's relative path table, which lists the directories the files are in.
 *
 * The member titles come from the labels OpenArkObject::PrintRelPaths() writes: "pathHash",
 * "flags", and "pathOffset". The path offset is rebased at mount time in the same way as a file
 * entry's name offset.
 */
struct ArkRelPath {
    unsigned short mPathHash; /*!< HashArkString() of the path. */
    unsigned short mFlags;    /*!< Flags whose meaning is unrecovered. */
    int mPathOffset;          /*!< Offset of the path inside the string table. */
};

/**
 * Hash a file name or a relative path for the table searches.
 *
 * Each character is shifted left by one more place than the character before it, the shift
 * wrapping from 7 back to 0, and combined into a 16-bit accumulator with exclusive or.
 *
 * @param pszName The string to hash.
 * @return The hash.
 * @ghidraAddress NTSC-U/C: 0x0055c340
 * @ghidraAddress PAL: 0x0059d560
 */
short HashArkString(const char *pszName);

class OpenArkObject;

/**
 * One open stream on a mounted archive.
 *
 * A handle with bit 0x4000 set identifies a stream inside an archive rather than a loose file.
 */
struct ArkStream {
    /**
     * Find the mounted archive this stream reads from.
     *
     * @return The archive whose file matches mFile, or null when none is mounted.
     * @ghidraAddress NTSC-U/C: 0x0055c1b8
     * @ghidraAddress PAL: 0x0059d3d8
     */
    OpenArkObject *FindArk() const;

    int mArkPosition;     /*!< Read position as a byte offset into the whole archive. */
    int mPosition;        /*!< Read position as a byte offset into the file. */
    int mFile;            /*!< The archive's file, which is what ties a stream to its archive. */
    int mHandle;          /*!< The stream handle, with bit 0x4000 cleared. */
    ArkFileEntry *mEntry; /*!< The file entry this stream reads. */
};

/**
 * Mounted ark archive.
 *
 * An ark stores the game's data files in one seekable blob, so the streaming layer can read a file
 * without a directory lookup per file. This class is not polymorphic and has no RTTI, so it has no
 * vptr. Its name comes from the debug strings of its dump routines, and its allocations use the
 * `ArkFile.cpp` tag. The record is 424 bytes and begins with an embedded HxStr. The mount
 * therefore assigns a path through the record pointer itself.
 *
 * A mount copies the whole 256-byte header into the record, then allocates one block for the file
 * table, the relative path table, and the string table together, and rebases every string offset
 * into that block. The header's own path is lowercased in place and must include a `run`
 * component, which is what fixes the archive's mount point.
 *
 * The header member titles come from the labels PrintHeader() writes. The data members are public
 * because the stream and sector routines of this unit (ArkStream::FindArk(), SeekArkStream(),
 * ArkfileGetBaseSector(), and ArkfileLogicalToPhysicalSector()) read them directly as free
 * functions.
 */
class OpenArkObject {
public:
    /**
     * Mount an ark archive by path.
     *
     * @param pszPath The archive path, relative to the data root.
     * @return Non-zero on success.
     * @ghidraAddress NTSC-U/C: 0x00559858
     * @ghidraAddress PAL: 0x0059a9b0
     */
    static int Open(const char *pszPath);

    /**
     * Construct an unmounted archive.
     *
     * Only the path and the four table pointers are cleared. mFile is deliberately not, which the
     * one unreachable branch of Open() depends on.
     */
    OpenArkObject();

    /**
     * Unmount a previously mounted archive.
     *
     * The unmount is refused, and reports zero, when the path is not mounted and when a stream is
     * still open on the archive's file. In the second case the offending stream record is erased
     * first.
     *
     * @param pszPath The same path that was passed to Open().
     * @return Non-zero when the archive was unmounted.
     * @ghidraAddress NTSC-U/C: 0x00559f70
     * @ghidraAddress PAL: 0x0059b180
     */
    static int Close(const char *pszPath);

    /**
     * Release the tables the mount allocated.
     *
     * The destructor is inlined into Close(), which is where its line number comes from.
     *
     * @ghidraAddress NTSC-U/C: 0x00559f70
     * @ghidraAddress PAL: 0x0059b180
     */
    ~OpenArkObject();

    /**
     * Find the entry of a file in whichever mounted archive the path maps to.
     *
     * The path is copied, with every backslash turned into a slash and every capital letter
     * lowercased, before OpenArkObject::MapPathToArkIndex() splits it and selects the archive. The
     * archive's file table is then searched for the name and relative path hashes, and the strings
     * confirm a hash match.
     *
     * @param pszPath The path to find.
     * @param pszName Receives the file name, at least kArkNameBufferSize bytes.
     * @param pszRelPath Receives the path relative to the archive's mount point, at least
     * kArkRelPathBufferSize bytes.
     * @param pnArk Receives the index of the archive in g_apMountedArks, or -1 when the file was
     * not found.
     * @return The file entry, or null when the file was not found.
     * @ghidraAddress NTSC-U/C: 0x0055a6d0
     * @ghidraAddress PAL: 0x0059b8f0
     */
    static ArkFileEntry *
    FindFileEntryByPath(const char *pszPath, char *pszName, char *pszRelPath, int *pnArk);

    /**
     * Open a stream on one file of this archive.
     *
     * The stream starts at the file's first byte, and its record is appended to gOpenArkFileTable.
     *
     * @param pEntry The file to read, one of this archive's file entries.
     * @return The new stream handle, without kFileHandleArkStream.
     * @ghidraAddress NTSC-U/C: 0x0055c288
     * @ghidraAddress PAL: 0x0059d4a8
     */
    int OpenStream(ArkFileEntry *pEntry);

    /**
     * Write every field of the copied archive header to standard output.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress NTSC-U/C: 0x0055aa38
     * @ghidraAddress PAL: 0x0059bc58
     */
    void PrintHeader() const;

    /**
     * Write every entry of the relative path table to standard output.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress NTSC-U/C: 0x0055ac30
     * @ghidraAddress PAL: 0x0059be50
     */
    void PrintRelPaths() const;

    /**
     * Write every entry of the file table to standard output.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress NTSC-U/C: 0x0055ada0
     * @ghidraAddress PAL: 0x0059bfc0
     */
    void PrintFiles() const;

    /**
     * Write every file name and every relative path to standard output.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress NTSC-U/C: 0x0055afc8
     * @ghidraAddress PAL: 0x0059c1e8
     */
    void DumpStrings() const;

    /**
     * Write the header, the file table, the relative path table, and the strings, in that order.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress NTSC-U/C: 0x0055c3a0
     * @ghidraAddress PAL: 0x0059d5c0
     */
    void PrintAll() const;

private:
    /**
     * Split a path and select the mounted archive whose mount point starts its directory.
     *
     * A path starting with a slash maps to no archive, and one starting with a backslash or a full
     * stop is fatal. The directory is the path up to its last slash, and the archives are tried
     * from the most recently mounted back. A match strips the mount point, and the slash after it,
     * from pszRelPath. The mount point is compared lowercased against the directory as it stands.
     * The strip counts the whole mount point even when the directory only continues it (a mount
     * point `a` matching the directory `ab`), and an archive whose mount point is empty matches
     * every directory.
     *
     * @param pszPath The path to split.
     * @param pszName Receives the file name.
     * @param pszRelPath Receives the directory, and then the path relative to the mount point.
     * @return The index of the archive in g_apMountedArks, or -1 when none matches.
     * @ghidraAddress NTSC-U/C: 0x0055a868
     * @ghidraAddress PAL: 0x0059ba88
     */
    static int MapPathToArkIndex(const char *pszPath, char *pszName, char *pszRelPath);

    /**
     * Search this archive's file table for a name in a relative path.
     *
     * The hashes are compared before the strings. Each hash argument is a sign-extended `short`
     * and each stored hash an `unsigned short`, which never matches a hash with its top bit set.
     * HashArkString() cannot produce one for a string of 7-bit characters.
     *
     * @param nNameHash HashArkString() of pszName.
     * @param nRelPathHash HashArkString() of pszRelPath.
     * @param pszName The file name.
     * @param pszRelPath The path relative to the mount point.
     * @return The file entry, or null when this archive does not list the file.
     * @ghidraAddress NTSC-U/C: 0x0055c050
     * @ghidraAddress PAL: 0x0059d270
     */
    ArkFileEntry *FindFileEntry(short nNameHash,
                                short nRelPathHash,
                                const char *pszName,
                                const char *pszRelPath) const;

public:
    HxStr mPath;           /*!< The path the archive was mounted by. */
    int mFile;             /*!< The archive's file, negative when the open failed. */
    char mSig[4];          /*!< The header signature. */
    int mVersion;          /*!< The header version, kArkVersion when accepted. */
    int mDirOffset;        /*!< Archive offset of the file table. */
    int mNumFiles;         /*!< Entries in the file table. */
    int mRelPathOffset;    /*!< Archive offset of the relative path table. */
    int mNumPaths;         /*!< Entries in the relative path table. */
    int mStringTabOffset;  /*!< Archive offset of the string table. */
    int mNumStrings;       /*!< Entries in the string table. */
    int mSizeHdrAndDir;    /*!< Archive offset one past the string table. */
    int mSectorSize;       /*!< Bytes in one archive chunk. */
    int mOptimized;        /*!< Set when the optimized block is present. */
    int mOptimizedOffset;  /*!< Archive offset of the optimized block. */
    int mOptimizedCount;   /*!< Two-byte entries in the optimized block. */
    int mHeaderReserved40; // +0x040 zero in every shipped archive, and never read
    int mHeaderReserved44; // +0x044 zero in every shipped archive, and never read
    int mHeaderReserved48; // +0x048 zero in every shipped archive, and never read
    char mHeaderPath[kArkHeaderSize - 0x40]; /*!< The header path, lowercased by the mount. */
    void *mTables;                           /*!< The one block the three tables live in. */
    ArkFileEntry *mFiles;                    /*!< The file table, at the start of mTables. */
    ArkRelPath *mRelPaths;                   /*!< The relative path table inside mTables. */
    char *mStrings;                          /*!< The string table inside mTables. */
    int mDiscLsn;                            /*!< The archive's start sector on the disc. */
    void *mOptimizedTable; /*!< The optimized block's logical chunk indices, or null. */
    int mOptimizedCursor;  /*!< The slot the next lookup tries first, or -1. */
    char mMountPoint[kArkMountPointSize]; /*!< The header path after its `run` component. */
};

/**
 * Erase a stream record by handle.
 *
 * @param nHandle The stream handle.
 * @return Zero when the record was erased, or -1 when no record has that handle.
 * @ghidraAddress NTSC-U/C: 0x0055a1a0
 * @ghidraAddress PAL: 0x0059b3c0
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
 * @ghidraAddress NTSC-U/C: 0x0055c158
 * @ghidraAddress PAL: 0x0059d378
 */
ArkStream *LookupOpenFile(int nHandle);

/**
 * Report the file entry a stream reads.
 *
 * The entry is what gives a caller the stream's stored and inflated sizes without a file table
 * search of its own. The search walks the stream vector here rather than through
 * LookupOpenFile(), and it masks kFileHandleArkStream off the handle in the same way.
 *
 * @param nHandle The stream handle, with the bit or without it.
 * @return The entry, or null when no record has that handle.
 * @ghidraAddress NTSC-U/C: 0x0055be80
 * @ghidraAddress PAL: 0x0059d0a0
 */
ArkFileEntry *FindOpenFileInArk(int nHandle);

/**
 * Open a stream on a file inside a mounted archive.
 *
 * @param pszPath The path of the file.
 * @return The stream handle, without kFileHandleArkStream, or -1 when no mounted archive lists the
 * file.
 * @ghidraAddress NTSC-U/C: 0x0055bce8
 * @ghidraAddress PAL: 0x0059cf08
 */
int OpenFileInArk(const char *pszPath);

/**
 * Report the stored length of a file inside a mounted archive.
 *
 * The shipped program does not call it.
 *
 * @param pszPath The path of the file.
 * @return The length in bytes, or -1 when no mounted archive lists the file.
 * @ghidraAddress NTSC-U/C: 0x0055bee0
 * @ghidraAddress PAL: 0x0059d100
 */
int GetArkFileLengthByPath(const char *pszPath);

/**
 * Read from an ark stream through the sector cache.
 *
 * The request is clamped to the bytes left in the file. Each 64 KiB chunk the read touches is
 * taken from the cache, and a chunk the cache does not have is read into the least recently used
 * row first, at the position ArkfileLogicalToPhysicalSector() maps it to. A cached chunk the
 * current asynchronous operation is still filling waits for that operation through AsyncCheck().
 *
 * @param nHandle The stream handle.
 * @param pBuffer The destination.
 * @param nBytes The number of bytes requested.
 * @return The number of bytes read, or -1 when no record has that handle or the stream is at the
 * end of its file.
 * @ghidraAddress NTSC-U/C: 0x0055a280
 * @ghidraAddress PAL: 0x0059b4a0
 */
int ReadArkStreamThroughCache(int nHandle, void *pBuffer, unsigned nBytes);

/**
 * Forward a control request to sceIoctl().
 *
 * The shipped program does not call it.
 *
 * @param nFile The file descriptor.
 * @param nRequest The request.
 * @param pArg The request argument.
 * @return The sceIoctl() result.
 * @ghidraAddress NTSC-U/C: 0x0055c3e0
 * @ghidraAddress PAL: 0x0059d600
 */
int IoctlFile(int nFile, int nRequest, void *pArg);

/**
 * Wait until no asynchronous operation is running on a file.
 *
 * The routine polls sceIoctl() with kSceFsExecuting and performs no other work between the polls.
 * A negative descriptor returns at once. The shipped program does not call it.
 *
 * @param nFile The file descriptor.
 * @ghidraAddress NTSC-U/C: 0x0055c458
 * @ghidraAddress PAL: 0x0059d678
 */
void WaitForFileIdle(int nFile);

/**
 * Report the archive a stream reads from.
 *
 * @param nHandle The stream handle.
 * @return The archive's file, or -1 when no record has that handle.
 * @ghidraAddress NTSC-U/C: 0x0055c000
 * @ghidraAddress PAL: 0x0059d220
 */
int GetArkfileIdFromFileFd(int nHandle);

/**
 * Report the disc sector a mounted archive starts at.
 *
 * A file that is not a mounted archive is fatal, and the report lists every mounted archive's file
 * before stopping.
 *
 * @param nFile The archive's file.
 * @return The archive's start sector.
 * @ghidraAddress NTSC-U/C: 0x0055a590
 * @ghidraAddress PAL: 0x0059b7b0
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
 * OpenArkObject::mOptimizedCursor makes the common case constant time. It records the slot after
 * the one the last lookup resolved. That is the slot a sequential read wants next. A miss on the
 * guess reports through the log, abandons the cursor, and falls back to searching the whole array.
 * A chunk that appears in no slot is fatal.
 *
 * A file that is not a mounted archive is reported through the log, and the argument is returned
 * unchanged.
 *
 * @param nFile The archive's file.
 * @param nSector The logical chunk index.
 * @return The chunk position on the disc.
 * @ghidraAddress NTSC-U/C: 0x0055a410
 * @ghidraAddress PAL: 0x0059b630
 */
int ArkfileLogicalToPhysicalSector(int nFile, int nSector);

/**
 * Mount the archives the game needs for the whole session.
 *
 * Performs no work when ark archives are not in use. The first archive that fails to mount is
 * reported on `cout`, and the archives after it are not tried.
 *
 * @return Non-zero when every archive was mounted.
 * @ghidraAddress NTSC-U/C: 0x004dfb20
 * @ghidraAddress PAL: 0x0051e280
 */
int InitArk();

/**
 * Unmount the archives InitArk() mounted.
 *
 * Each archive is closed through OpenArkObject::Close() even when an earlier one was refused. The
 * result counts the unmounts, so it is zero after a successful teardown. It is non-zero only when
 * ark archives are not in use or when none of the three was still mounted.
 *
 * @return Non-zero when no archive was unmounted.
 * @ghidraAddress NTSC-U/C: 0x004dfbd8
 * @ghidraAddress PAL: 0x0051e338
 */
int CloseArk();

/** The number of archives in g_apSessionArkPaths. */
constexpr int kSessionArkCount = 3;

/**
 * The archives InitArk() mounts for the session and CloseArk() unmounts: `ark/root.ark`,
 * `ark/levels.ark`, and `ark/arenas.ark`.
 *
 * @ghidraAddress NTSC-U/C: 0x00702650
 * @ghidraAddress PAL: 0x007460a0
 */
extern const char *const g_apSessionArkPaths[kSessionArkCount];

/**
 * Every archive mounted right now.
 *
 * @ghidraAddress NTSC-U/C: 0x00725e90
 * @ghidraAddress PAL: 0x00769b30
 */
extern std::vector<OpenArkObject *> g_apMountedArks;

/**
 * Every stream open on a mounted archive.
 *
 * @ghidraAddress NTSC-U/C: 0x00725ea0
 * @ghidraAddress PAL: 0x00769b40
 */
extern std::vector<ArkStream> gOpenArkFileTable;

/**
 * Handle OpenArkObject::OpenStream() gives the next stream, incremented after every open.
 *
 * @ghidraAddress NTSC-U/C: 0x00725e88
 * @ghidraAddress PAL: 0x00769b28
 */
extern int gOpenFileIndex;
