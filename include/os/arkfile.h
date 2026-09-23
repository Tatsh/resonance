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

/** Bytes of the lowercased path copy ArkFile::FindFileEntryByPath() builds on its stack. */
constexpr int kArkPathBufferSize = 0x80;

/** Bytes every caller of ArkFile::FindFileEntryByPath() reserves for the file name. */
constexpr int kArkNameBufferSize = 0x80;

/** Bytes every caller of ArkFile::FindFileEntryByPath() reserves for the relative path. */
constexpr int kArkRelPathBufferSize = 0x100;

/**
 * Archive index that asks ArkFile::FindFileEntryByPath() to search every mounted archive.
 *
 * ArkFile::MapPathToArkIndex() never reports it, and the search is therefore unreachable.
 */
constexpr int kArkIndexAny = 100;

/**
 * One entry of the archive's file table.
 *
 * The member titles come from the labels ArkFile::DumpFiles() writes: "nameHash", "flags",
 * "nameOffset", "relPathIndex", "sectorOffset", "sector", and "length". The dump does not write
 * mSize, and its title comes from the one reader, the asynchronous loader, which takes it as the
 * inflated size. The name offset is rebased at mount time from an archive-relative offset to an
 * offset inside the string table.
 */
struct ArkFileEntry {
    unsigned short mNameHash;     /*!< ArkFile::HashName() of the file name. */
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
 * The member titles come from the labels ArkFile::DumpRelativePaths() writes: "pathHash", "flags",
 * and "pathOffset". The path offset is rebased at mount time in the same way as a file entry's
 * name offset.
 */
struct ArkRelPath {
    unsigned short mPathHash; /*!< ArkFile::HashName() of the path. */
    unsigned short mFlags;    /*!< Flags whose meaning is unrecovered. */
    int mPathOffset;          /*!< Offset of the path inside the string table. */
};

class ArkFile;

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
     * @ghidraAddress 0x0055c1b8
     */
    ArkFile *FindArk() const;

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
 * vptr. Its name comes from the `ArkFile.cpp` allocation tag. The record is 424 bytes and begins
 * with an embedded HxStr, which is why the mount assigns a path through the record pointer itself.
 *
 * A mount copies the whole 256-byte header into the record, then allocates one block for the file
 * table, the relative path table, and the string table together, and rebases every string offset
 * into that block. The header's own path is lowercased in place and must include a `run`
 * component, which is what fixes the archive's mount point.
 *
 * The header member titles come from the labels DumpHeader() writes, and the debug strings of the
 * dump routines call the record `OpenArkObject`. Every data member is private, and ArkStream is a
 * friend for the one routine that matches a stream to its archive.
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

    /**
     * Find the entry of a file in whichever mounted archive the path maps to.
     *
     * The path is copied, with every backslash turned into a slash and every capital letter
     * lowercased, before ArkFile::MapPathToArkIndex() splits it and selects the archive. The
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
     * @ghidraAddress 0x0055a6d0
     */
    static ArkFileEntry *
    FindFileEntryByPath(const char *pszPath, char *pszName, char *pszRelPath, int *pnArk);

    /**
     * Open a stream on one file of this archive.
     *
     * The stream starts at the file's first byte, and its record is appended to g_aArkStreams.
     *
     * @param pEntry The file to read, one of this archive's file entries.
     * @return The new stream handle, without kFileHandleArkStream.
     * @ghidraAddress 0x0055c288
     */
    int OpenStream(ArkFileEntry *pEntry);

    /**
     * Write every field of the copied archive header to standard output.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress 0x0055aa38
     */
    void DumpHeader() const;

    /**
     * Write every entry of the relative path table to standard output.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress 0x0055ac30
     */
    void DumpRelativePaths() const;

    /**
     * Write every entry of the file table to standard output.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress 0x0055ada0
     */
    void DumpFiles() const;

    /**
     * Write every file name and every relative path to standard output.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress 0x0055afc8
     */
    void DumpStrings() const;

    /**
     * Write the header, the file table, the relative path table, and the strings, in that order.
     *
     * The shipped program does not call it.
     *
     * @ghidraAddress 0x0055c3a0
     */
    void Dump() const;

private:
    friend struct ArkStream;

    /**
     * Hash a file name or a relative path for the table searches.
     *
     * Each character is shifted left by one more place than the character before it, the shift
     * wrapping from 7 back to 0, and combined into a 16-bit accumulator with exclusive or.
     *
     * @param pszName The string to hash.
     * @return The hash.
     * @ghidraAddress 0x0055c340
     */
    static short HashName(const char *pszName);

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
     * @ghidraAddress 0x0055a868
     */
    static int MapPathToArkIndex(const char *pszPath, char *pszName, char *pszRelPath);

    /**
     * Search this archive's file table for a name in a relative path.
     *
     * The hashes are compared before the strings. Each hash argument is a sign-extended `short`
     * and each stored hash an `unsigned short`, which never matches a hash with its top bit set.
     * HashName() cannot produce one for a string of 7-bit characters.
     *
     * @param nNameHash HashName() of pszName.
     * @param nRelPathHash HashName() of pszRelPath.
     * @param pszName The file name.
     * @param pszRelPath The path relative to the mount point.
     * @return The file entry, or null when this archive does not list the file.
     * @ghidraAddress 0x0055c050
     */
    ArkFileEntry *FindFileEntry(short nNameHash,
                                short nRelPathHash,
                                const char *pszName,
                                const char *pszRelPath) const;

    HxStr mPath;
    int mFile; // negative when the open failed
    char mSig[4];
    int mVersion;
    int mDirOffset; // archive offset of the file table
    int mNumFiles;
    int mRelPathOffset; // archive offset of the relative path table
    int mNumPaths;
    int mStringTabOffset; // archive offset of the string table
    int mNumStrings;
    int mSizeHdrAndDir; // archive offset one past the string table
    int mSectorSize;
    int mOptimized; // set when the optimized block is present
    int mOptimizedOffset;
    int mOptimizedCount;                     // entries, each two bytes
    int mHeaderUnknown40;                    // +0x040
    int mHeaderUnknown44;                    // +0x044
    int mHeaderUnknown48;                    // +0x048
    char mHeaderPath[kArkHeaderSize - 0x40]; // lowercased in place by the mount
    void *mTables;                           // the one block the three tables live in
    ArkFileEntry *mFiles;                    // mTables
    ArkRelPath *mRelPaths;                   // mTables plus the relative path table's offset
    char *mStrings;                          // mTables plus the string table's offset
    int mDiscLsn;                            // the archive's start sector on the disc
    void *mOptimizedTable;
    int mOptimizedCursor;                 // slot the next lookup tries first, or -1
    char mMountPoint[kArkMountPointSize]; // the header path after its `run` component
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
 * Report the file entry a stream reads.
 *
 * The entry is what gives a caller the stream's stored and inflated sizes without a file table
 * search of its own. The search walks the stream vector here rather than through
 * FindOpenArkStream(), and it masks kFileHandleArkStream off the handle in the same way.
 *
 * @param nHandle The stream handle, with the bit or without it.
 * @return The entry, or null when no record has that handle.
 * @ghidraAddress 0x0055be80
 */
ArkFileEntry *GetArkStreamFileEntry(int nHandle);

/**
 * Open a stream on a file inside a mounted archive.
 *
 * @param pszPath The path of the file.
 * @return The stream handle, without kFileHandleArkStream, or -1 when no mounted archive lists the
 * file.
 * @ghidraAddress 0x0055bce8
 */
int LookupArkStreamForPath(const char *pszPath);

/**
 * Report the stored length of a file inside a mounted archive.
 *
 * The shipped program does not call it.
 *
 * @param pszPath The path of the file.
 * @return The length in bytes, or -1 when no mounted archive lists the file.
 * @ghidraAddress 0x0055bee0
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
 * @ghidraAddress 0x0055a280
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
 * @ghidraAddress 0x0055c3e0
 */
int IoctlFile(int nFile, int nRequest, void *pArg);

/**
 * Wait until no asynchronous operation is running on a file.
 *
 * The routine polls sceIoctl() with kSceFsExecuting and performs no other work between the polls.
 * A negative descriptor returns at once. The shipped program does not call it.
 *
 * @param nFile The file descriptor.
 * @ghidraAddress 0x0055c458
 */
void WaitForFileIdle(int nFile);

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
 * Unmount the archives InitArk() mounted.
 *
 * Each archive is closed through ArkFile::Close() even when an earlier one was refused. The result
 * counts the unmounts, so it is zero after a successful teardown. It is non-zero only when ark
 * archives are not in use or when none of the three was still mounted.
 *
 * @return Non-zero when no archive was unmounted.
 * @ghidraAddress 0x004dfbd8
 */
int CloseArk();

/** The number of archives in g_apSessionArkPaths. */
constexpr int kSessionArkCount = 3;

/**
 * The archives InitArk() mounts for the session and CloseArk() unmounts: `ark/root.ark`,
 * `ark/levels.ark`, and `ark/arenas.ark`.
 *
 * @ghidraAddress 0x00702650
 */
extern const char *const g_apSessionArkPaths[kSessionArkCount];

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
 * Handle ArkFile::OpenStream() gives the next stream, incremented after every open.
 *
 * @ghidraAddress 0x00725e88
 */
extern int g_nNextArkStreamHandle;
