#include "os/openarkobject.h"

#include <ctype.h>
#include <iostream>
#include <libcdvd.h>
#include <sifdev.h>
#include <string.h>
#include <vector>

#include "os/async.h"
#include "os/fileio.h"
#include "os/hostmode.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/seccache.h"

namespace {

// Chunk of the archive the header is read out of.
constexpr int kArkHeaderSector = 0;

// HashArkString() shifts each character by one more place than the last, wrapping after 7.
constexpr int kArkHashShiftMask = 7;

// Line every dump closes with, one literal shared by all four.
constexpr char kArkDumpRule[] = "===========================================================";

// The report ArkfileGetBaseSector() writes to the log and then passes to Fatal().
constexpr char kBaseSectorMissingFormat[] =
    "ArkfileGetBaseSector: can't find arkfile with id: %d\n";

// The sceOpen() mode OpenStreamByPath() passes, which opens for reading only.
constexpr int kOpenReadOnly = 1;

// The stream table search LookupOpenFile() performs and the two entry accessors expand in place.
inline ArkStream *FindStreamRecord(int nHandle) {
    nHandle &= ~kFileHandleArkStream;
    const int nStreams = gOpenArkFileTable.size();
    for (int i = 0; i < nStreams; ++i) {
        if (gOpenArkFileTable[i].mHandle == nHandle) {
            return &gOpenArkFileTable[i];
        }
    }
    return nullptr;
}

// Set once the sector cache has been brought up, so that only the first mount does it.
// NTSC-U/C: 0x00725eb0, PAL: 0x00769b50
int g_bArkSectorCacheReady;

// Both disc modes build the same path, and both the host mode and the disc mode's fallback build
// the same one, so each build appears twice in the image.
void BuildDiscPath(char *pszPath, const HxStr &name) {
    strcpy(pszPath, kArkDiscRoot);
    FilenameToISO9660(name.mStr != nullptr ? name.mStr : g_szEmptyString, pszPath);
}

void BuildHostPath(char *pszPath, const char *pszArchive) {
    strcpy(pszPath, kArkHostRoot);
    strcat(pszPath, pszArchive);
}

// Finds the `run` component that fixes the mount point. The component matches only when a
// separator or the terminator follows it, so a directory such as `running` is not mistaken for it.
const char *FindRunComponent(const char *pszPath) {
    for (const char *p = pszPath; *p != '\0'; ++p) {
        if (p[0] != 'r' || p[1] != 'u' || p[2] != 'n') {
            continue;
        }
        if (p[3] == '\0' || p[3] == '/') {
            return p;
        }
    }
    return nullptr;
}

} // namespace

OpenArkObject::OpenArkObject()
    : mPath(nullptr), mTables(nullptr), mFiles(nullptr), mRelPaths(nullptr), mStrings(nullptr) {
}

OpenArkObject::~OpenArkObject() {
    if (mTables != nullptr) {
        MemFreeTagged(mTables, __FILE__, __LINE__);
    }
    if (mOptimizedTable != nullptr) {
        MemFreeTagged(mOptimizedTable, __FILE__, __LINE__);
    }
}

// NTSC-U/C: 0x00559858, PAL: 0x0059a9b0
int OpenArkObject::Open(const char *pszPath) {
    HxStr strName(nullptr);
    HxStr strPath(pszPath);

    if (g_bArkSectorCacheReady == 0) {
        SectorCacheInit(kArkSectorCacheRows);
        g_bArkSectorCacheReady = 1;
    }
    AsyncCheck(1);

    // Every failure below returns without releasing this record, so a failed mount leaks it.
    OpenArkObject *pArk = new OpenArkObject;

    int nSlash = strPath.ReverseFind('/');
    strName = strPath.Mid(nSlash + 1, strPath.mLen - nSlash - 1);

    char szDevice[kArkDevicePathSize];
    int nMode = GetHostMode();
    if (nMode == kHostModeCdOnly) {
        BuildDiscPath(szDevice, strName);
        pArk->mFile = OpenStreamByPath(szDevice);
    } else if (nMode == kHostModeCdHost) {
        BuildDiscPath(szDevice, strName);
        pArk->mFile = OpenStreamByPath(szDevice);
        if (pArk->mFile < 0) {
            BuildHostPath(szDevice, pszPath);
            pArk->mFile = OpenStreamByPath(szDevice);
        }
    } else if (nMode == kHostModeHostOnly) {
        BuildHostPath(szDevice, pszPath);
        pArk->mFile = OpenStreamByPath(szDevice);
    }
    // A mode outside those three would test an mFile the constructor never set. GetHostMode()
    // reports only those three, so the path is unreachable.
    if (pArk->mFile < 0) {
        return 0;
    }

    pArk->mPath = pszPath;

    if (UsingCdMedia() != 0) {
        sceCdlFILE cdFile;
        if (sceCdSearchFile(&cdFile, strchr(szDevice, ':') + 1) == 0) {
            printf("sceCdSearchFile failed on: %s\n", szDevice);
            return 0;
        }
        pArk->mDiscLsn = cdFile.lsn;
    }

    pArk->mOptimized = 0;
    g_apMountedArks.push_back(pArk);

    // Yes, the binary discards this lookup's result and claims a row unconditionally.
    SectorCacheFind(pArk->mFile, kArkHeaderSector);
    SectorCacheRow *pRow = SectorCacheGetLRU(pArk->mFile, kArkHeaderSector);
    ReadStreamChunk(pArk->mFile, kArkHeaderSector, pRow->mBuffer, kSectorCacheRowSize);
    memcpy(pArk->mSig, pRow->mBuffer, kArkHeaderSize);

    if (pArk->mVersion != kArkVersion) {
        printf("ERROR - ARKFILE VERSION INCORRECT - PLEASE REGENERATE!\n");
        return 0;
    }

    if (pArk->mOptimized != 0) {
        printf("FOUND OPTIMIZED ARKFILE %s, optimized flag: %d\n", pszPath, pArk->mOptimized);
        unsigned nOptimized = pArk->mOptimizedCount * sizeof(unsigned short);
        pArk->mOptimizedTable = MemAllocTagged(nOptimized, __FILE__, __LINE__);
        memcpy(pArk->mOptimizedTable,
               static_cast<char *>(pRow->mBuffer) + pArk->mOptimizedOffset,
               nOptimized);
        // Slot 1 rather than slot 0, because the mount above has already read the chunk slot 0
        // addresses.
        pArk->mOptimizedCursor = 1;
    } else {
        pArk->mOptimizedTable = nullptr;
    }

    unsigned nTables = pArk->mSizeHdrAndDir - pArk->mDirOffset;
    pArk->mTables = MemAllocTagged(nTables, __FILE__, __LINE__);
    memcpy(pArk->mTables, static_cast<char *>(pRow->mBuffer) + pArk->mDirOffset, nTables);

    // Each table's string offsets arrive relative to the archive and are rebased onto the string
    // table.
    pArk->mFiles = static_cast<ArkFileEntry *>(pArk->mTables);
    for (int i = 0; i < pArk->mNumFiles; ++i) {
        pArk->mFiles[i].mNameOffset -= pArk->mStringTabOffset;
    }

    pArk->mRelPaths = reinterpret_cast<ArkRelPath *>(static_cast<char *>(pArk->mTables) +
                                                     pArk->mRelPathOffset - pArk->mDirOffset);
    for (int i = 0; i < pArk->mNumPaths; ++i) {
        pArk->mRelPaths[i].mPathOffset -= pArk->mStringTabOffset;
    }

    pArk->mStrings = static_cast<char *>(pArk->mTables) + pArk->mStringTabOffset - pArk->mDirOffset;

    for (char *p = pArk->mHeaderPath; *p != '\0'; ++p) {
        *p = tolower(*p);
    }

    const char *pszRun = FindRunComponent(pArk->mHeaderPath);
    if (pszRun == nullptr) {
        printf("ERROR - BAD PATH IN ARKFILE HEADER FOR ARKFILE: %s\n", pszPath);
        return 0;
    }

    pszRun += sizeof(kArkRunComponent) - 1;
    strcpy(pArk->mMountPoint, *pszRun == '/' ? pszRun + 1 : pszRun);
    return 1;
}

// NTSC-U/C: 0x00559f70, PAL: 0x0059b180
int OpenArkObject::Close(const char *pszPath) {
    unsigned nArk = 0;
    while (nArk < g_apMountedArks.size()) {
        const char *pszMounted = g_apMountedArks[nArk]->mPath.mStr;
        if (pszMounted == nullptr) {
            pszMounted = g_szEmptyString;
        }
        if (strcmp(pszMounted, pszPath) == 0) {
            break;
        }
        ++nArk;
    }
    if (nArk == g_apMountedArks.size()) {
        return 0;
    }

    // A stream still open on the archive's file blocks the unmount. The offending record is erased
    // before the refusal, which is what allows a later attempt to succeed.
    for (unsigned i = 0; i < gOpenArkFileTable.size(); ++i) {
        if (gOpenArkFileTable[i].mFile == g_apMountedArks[nArk]->mFile) {
            EraseArkStream(gOpenArkFileTable[i].mHandle);
            return 0;
        }
    }

    CloseLoadFile(g_apMountedArks[nArk]->mFile);
    SectorCacheRemove(g_apMountedArks[nArk]->mFile);
    delete g_apMountedArks[nArk];
    g_apMountedArks.erase(g_apMountedArks.begin() + nArk);
    return 1;
}

// NTSC-U/C: 0x0055a1a0, PAL: 0x0059b3c0
int EraseArkStream(int nHandle) {
    for (unsigned i = 0; i < gOpenArkFileTable.size(); ++i) {
        if (gOpenArkFileTable[i].mHandle == nHandle) {
            gOpenArkFileTable.erase(gOpenArkFileTable.begin() + i);
            return 0;
        }
    }
    return -1;
}

// NTSC-U/C: 0x00725e88, PAL: 0x00769b28
int gOpenFileIndex = 1;

// NTSC-U/C: 0x0055c158, PAL: 0x0059d378
ArkStream *LookupOpenFile(int nHandle) {
    return FindStreamRecord(nHandle);
}

// NTSC-U/C: 0x0055be80, PAL: 0x0059d0a0
ArkFileEntry *FindOpenFileInArk(int nHandle) {
    const ArkStream *pStream = FindStreamRecord(nHandle);
    return (pStream != nullptr) ? pStream->mEntry : nullptr;
}

// NTSC-U/C: 0x0055bf88, PAL: 0x0059d1a8
int FindOpenFileInArkTrueSize(int nStream) {
    const ArkStream *pStream = FindStreamRecord(nStream);
    const ArkFileEntry *pEntry = (pStream != nullptr) ? pStream->mEntry : nullptr;
    if (pEntry == nullptr) {
        return -1;
    }
    return pEntry->mSize;
}

// NTSC-U/C: 0x0055c000, PAL: 0x0059d220
int GetArkfileIdFromFileFd(int nHandle) {
    const ArkStream *pStream = LookupOpenFile(nHandle);
    if (pStream == nullptr) {
        return -1;
    }
    return pStream->mFile;
}

// NTSC-U/C: 0x0055c028, PAL: 0x0059d248
int ArkfileGetCurrAbsOffset(int nStream) {
    const ArkStream *pStream = LookupOpenFile(nStream);
    if (pStream == nullptr) {
        return -1;
    }
    return pStream->mArkPosition;
}

// NTSC-U/C: 0x0055bd38, PAL: 0x0059cf58
int SeekArkStream(int nStream, int nOffset, int nOrigin) {
    ArkStream *pStream = LookupOpenFile(nStream);
    if (pStream == nullptr) {
        return -1;
    }
    const OpenArkObject *pArk = pStream->FindArk();
    if (pArk == nullptr) {
        return -1;
    }

    const ArkFileEntry *pEntry = pStream->mEntry;
    const int nStart = pEntry->mSector * pArk->mSectorSize + pEntry->mSectorOffset;
    switch (nOrigin) {
    case kFileSeekSet:
        pStream->mPosition = nOffset;
        pStream->mArkPosition = nStart + nOffset;
        break;
    case kFileSeekCur:
        pStream->mArkPosition += nOffset;
        pStream->mPosition += nOffset;
        break;
    case kFileSeekEnd:
        pStream->mArkPosition = nStart + pEntry->mLength + nOffset;
        pStream->mPosition = pEntry->mLength + nOffset;
        break;
    default:
        return -1;
    }

    if (pStream->mPosition < 0) {
        pStream->mArkPosition -= pStream->mPosition;
        pStream->mPosition = 0;
    }
    return pStream->mPosition;
}

// NTSC-U/C: 0x0055a590, PAL: 0x0059b7b0
int ArkfileGetBaseSector(int nFile) {
    const unsigned nArks = g_apMountedArks.size();
    for (unsigned i = 0; i < nArks; ++i) {
        if (g_apMountedArks[i]->mFile == nFile) {
            return g_apMountedArks[i]->mDiscLsn;
        }
    }

    printf(kBaseSectorMissingFormat, nFile);
    printf("   (ark file table size: %d\n", g_apMountedArks.size());
    for (unsigned i = 0; i < g_apMountedArks.size(); ++i) {
        printf("   (ark id at index %d: %d\n", i, g_apMountedArks[i]->mFile);
    }
    Fatal(kBaseSectorMissingFormat, nFile);
    return 0;
}

// NTSC-U/C: 0x0055a410, PAL: 0x0059b630
int ArkfileLogicalToPhysicalSector(int nFile, int nSector) {
    for (unsigned i = 0; i < g_apMountedArks.size(); ++i) {
        OpenArkObject *pArk = g_apMountedArks[i];
        if (pArk->mFile != nFile) {
            continue;
        }
        if (pArk->mOptimized == 0) {
            return nSector;
        }

        const short *pTable = static_cast<const short *>(pArk->mOptimizedTable);
        const int nCursor = pArk->mOptimizedCursor;
        if (nCursor >= 0) {
            if (pTable[nCursor] == nSector) {
                pArk->mOptimizedCursor = nCursor + 1;
                return nCursor;
            }
            if (nCursor < pArk->mOptimizedCount) {
                printf("OPTIMIZED ARKFILE ORDERING FAILURE AT INDEX: %d\n", nCursor);
            }
            pArk->mOptimizedCursor = -1;
        }

        for (int nSlot = 0; nSlot < pArk->mOptimizedCount; ++nSlot) {
            if (pTable[nSlot] == nSector) {
                return nSlot;
            }
        }
        Fatal("ArkfileLogicalToPhysicalSector: can't find sector: %d\n", nSector);
    }

    printf("HEY!!! ArkfileLogicalToPhysicalSector can't find arkId: %d\n", nFile);
    return nSector;
}

// NTSC-U/C: 0x0055c400, PAL: 0x0059d620
int OpenStreamByPath(const char *pszPath) {
    sceCdSync(SCECdBlock);
    return sceOpen(pszPath, kOpenReadOnly);
}

// NTSC-U/C: 0x0055c438, PAL: 0x0059d658
void CloseLoadFile(int nFile) {
    sceClose(nFile);
}

// NTSC-U/C: 0x0055c498, PAL: 0x0059d6b8
void ReadStreamChunk(int nFile, int nSector, void *pBuffer, unsigned nLength) {
    // Yes, the binary maps the chunk again although ReadArkStreamThroughCache() already has.
    const int nPhysical = ArkfileLogicalToPhysicalSector(nFile, nSector);
    sceCdSync(SCECdBlock);
    sceLseek(nFile, nPhysical * kSectorCacheRowSize, SCE_SEEK_SET);
    sceRead(nFile, pBuffer, nLength);
}

// NTSC-U/C: 0x0055c340, PAL: 0x0059d560
short HashArkString(const char *pszName) {
    unsigned short nHash = 0;
    int nShift = 0;
    for (; *pszName != '\0'; ++pszName) {
        nHash ^= static_cast<unsigned short>(*pszName) << nShift;
        nShift = (nShift + 1) & kArkHashShiftMask;
    }
    return static_cast<short>(nHash);
}

// NTSC-U/C: 0x0055c050, PAL: 0x0059d270
ArkFileEntry *OpenArkObject::FindFileEntry(short nNameHash,
                                           short nRelPathHash,
                                           const char *pszName,
                                           const char *pszRelPath) const {
    ArkFileEntry *pEntry = mFiles;
    for (int i = 0; i < mNumFiles; ++i, ++pEntry) {
        const ArkRelPath &relPath = mRelPaths[pEntry->mRelPathIndex];
        if (nNameHash == pEntry->mNameHash && nRelPathHash == relPath.mPathHash &&
            strcmp(pszName, mStrings + pEntry->mNameOffset) == 0 &&
            strcmp(pszRelPath, mStrings + relPath.mPathOffset) == 0) {
            return pEntry;
        }
    }
    return nullptr;
}

// NTSC-U/C: 0x0055a868, PAL: 0x0059ba88
int OpenArkObject::MapPathToArkIndex(const char *pszPath, char *pszName, char *pszRelPath) {
    if (pszPath[0] == '/') {
        return -1;
    }
    if (pszPath[0] == '\\' || pszPath[0] == '.') {
        Fatal("Arkfile crisis: Unexpected arkfile path: %s\n", pszPath);
    }

    const char *pszSlash = strrchr(pszPath, '/');
    if (pszSlash == nullptr) {
        strcpy(pszName, pszPath);
        pszRelPath[0] = '\0';
    } else {
        const int nDirLength = pszSlash - pszPath;
        if (nDirLength > 0) {
            memcpy(pszRelPath, pszPath, nDirLength);
        }
        pszRelPath[nDirLength] = '\0';
        strcpy(pszName, pszSlash + 1);
    }

    for (int i = g_apMountedArks.size() - 1; i >= 0; --i) {
        const OpenArkObject *pArk = g_apMountedArks[i];
        int nLength = strlen(pArk->mMountPoint);
        int j = 0;
        while (j < nLength && tolower(pArk->mMountPoint[j]) == pszRelPath[j]) {
            ++j;
        }
        if (j != nLength) {
            continue;
        }
        if (nLength > 0) {
            if (pszRelPath[nLength] == '/') {
                ++nLength;
            }
            memmove(pszRelPath, pszRelPath + nLength, strlen(pszRelPath) - nLength + 1);
        }
        return i;
    }
    return -1;
}

// NTSC-U/C: 0x0055a6d0, PAL: 0x0059b8f0
ArkFileEntry *OpenArkObject::FindFileEntryByPath(const char *pszPath,
                                                 char *pszName,
                                                 char *pszRelPath,
                                                 int *pnArk) {
    char szPath[kArkPathBufferSize];
    (void)strlen(pszPath); // Yes, the binary discards this call's result.
    strcpy(szPath, pszPath);
    for (char *p = szPath; *p != '\0'; ++p) {
        if (*p == '\\') {
            *p = '/';
        } else if (*p >= 'A' && *p <= 'Z') {
            *p += 'a' - 'A';
        }
    }

    *pnArk = MapPathToArkIndex(szPath, pszName, pszRelPath);
    if (*pnArk < 0) {
        return nullptr;
    }
    const short nNameHash = HashArkString(pszName);
    const short nRelPathHash = HashArkString(pszRelPath);

    ArkFileEntry *pEntry;
    if (*pnArk == kArkIndexAny) {
        // A hit here reports kArkIndexAny as the index rather than the archive it was found in.
        const int nArks = g_apMountedArks.size();
        for (int i = 0; i < nArks; ++i) {
            if ((pEntry = g_apMountedArks[i]->FindFileEntry(
                     nNameHash, nRelPathHash, pszName, pszRelPath)) != nullptr) {
                return pEntry;
            }
        }
    } else if ((pEntry = g_apMountedArks[*pnArk]->FindFileEntry(
                    nNameHash, nRelPathHash, pszName, pszRelPath)) != nullptr) {
        return pEntry;
    }
    *pnArk = -1;
    return nullptr;
}

// NTSC-U/C: 0x0055c288, PAL: 0x0059d4a8
int OpenArkObject::OpenStream(ArkFileEntry *pEntry) {
    ArkStream stream;
    stream.mArkPosition = pEntry->mSector * mSectorSize + pEntry->mSectorOffset;
    stream.mPosition = 0;
    stream.mFile = mFile;
    stream.mHandle = gOpenFileIndex++;
    stream.mEntry = pEntry;
    gOpenArkFileTable.push_back(stream);
    return stream.mHandle;
}

// NTSC-U/C: 0x0055c1b8, PAL: 0x0059d3d8
OpenArkObject *ArkStream::FindArk() const {
    const int nArks = g_apMountedArks.size();
    for (int i = 0; i < nArks; ++i) {
        if (g_apMountedArks[i]->mFile == mFile) {
            return g_apMountedArks[i];
        }
    }
    return nullptr;
}

// NTSC-U/C: 0x0055bce8, PAL: 0x0059cf08
int OpenFileInArk(const char *pszPath) {
    char szName[kArkNameBufferSize];
    char szRelPath[kArkRelPathBufferSize];
    int nArk;
    ArkFileEntry *pEntry = OpenArkObject::FindFileEntryByPath(pszPath, szName, szRelPath, &nArk);
    if (pEntry == nullptr) {
        return -1;
    }
    return g_apMountedArks[nArk]->OpenStream(pEntry);
}

// NTSC-U/C: 0x0055bee0, PAL: 0x0059d100
int GetArkFileLengthByPath(const char *pszPath) {
    char szName[kArkNameBufferSize];
    char szRelPath[kArkRelPathBufferSize];
    int nArk;
    ArkFileEntry *pEntry = OpenArkObject::FindFileEntryByPath(pszPath, szName, szRelPath, &nArk);
    if (pEntry == nullptr) {
        return -1;
    }
    return pEntry->mLength;
}

// NTSC-U/C: 0x0055a280, PAL: 0x0059b4a0
int ReadArkStreamThroughCache(int nHandle, void *pBuffer, unsigned nBytes) {
    ArkStream *pStream = LookupOpenFile(nHandle);
    if (pStream == nullptr) {
        return -1;
    }
    const int nRemaining = pStream->mEntry->mLength - pStream->mPosition;
    if (nRemaining <= 0) {
        return -1;
    }
    if (static_cast<unsigned>(nRemaining) < nBytes) {
        nBytes = nRemaining;
    }

    char *pDest = static_cast<char *>(pBuffer);
    int nChunk = pStream->mArkPosition / kSectorCacheRowSize;
    int nChunkOffset = pStream->mArkPosition & (kSectorCacheRowSize - 1);
    const int nFile = pStream->mFile;
    for (int nLeft = nBytes; nLeft > 0;) {
        SectorCacheRow *pRow = SectorCacheFind(nFile, nChunk);
        if (pRow == nullptr) {
            pRow = SectorCacheGetLRU(nFile, nChunk);
            AsyncCheck(1);
            ReadStreamChunk(nFile,
                            ArkfileLogicalToPhysicalSector(nFile, nChunk),
                            pRow->mBuffer,
                            kSectorCacheRowSize);
        } else if (MatchesCurrentAsyncOp(nFile, nChunk) != 0) {
            AsyncCheck(1);
        }

        int nCopy = kSectorCacheRowSize - nChunkOffset;
        if (nLeft < nCopy) {
            nCopy = nLeft;
        }
        memcpy(pDest, static_cast<char *>(pRow->mBuffer) + nChunkOffset, nCopy);
        ++nChunk;
        nLeft -= nCopy;
        pDest += nCopy;
        nChunkOffset = 0;
    }

    pStream->mArkPosition += nBytes;
    pStream->mPosition += nBytes;
    return nBytes;
}

// NTSC-U/C: 0x0055aa38, PAL: 0x0059bc58
void OpenArkObject::PrintHeader() const {
    std::cout << "================== OpenArkObject Header ===================" << std::endl
              << " sig             " << mSig << std::endl
              << " version         " << mVersion << std::endl
              << " dirOffset       " << mDirOffset << std::endl
              << " numFiles        " << mNumFiles << std::endl
              << " relPathOffset   " << mRelPathOffset << std::endl
              << " numPaths        " << mNumPaths << std::endl
              << " stringTabOffset " << mStringTabOffset << std::endl
              << " numStrings      " << mNumStrings << std::endl
              << " sizeHdrAndDir   " << mSizeHdrAndDir << std::endl
              << " sectorSize      " << mSectorSize << std::endl
              << " path            " << mHeaderPath << std::endl
              << kArkDumpRule << std::endl
              << std::endl;
}

// NTSC-U/C: 0x0055ac30, PAL: 0x0059be50
void OpenArkObject::PrintRelPaths() const {
    std::cout << "============== OpenArkObject Relative Paths ===============" << std::endl;
    for (int i = 0; i < mNumPaths; ++i) {
        std::cout << "{ pathHash       " << mRelPaths[i].mPathHash << std::endl
                  << "  flags          " << mRelPaths[i].mFlags << std::endl
                  << "  pathOffset     " << mRelPaths[i].mPathOffset << " }" << std::endl;
    }
    std::cout << kArkDumpRule << std::endl << std::endl;
}

// NTSC-U/C: 0x0055ada0, PAL: 0x0059bfc0
void OpenArkObject::PrintFiles() const {
    std::cout << "================== OpenArkObject Files ====================" << std::endl;
    for (int i = 0; i < mNumFiles; ++i) {
        std::cout << "{ nameHash        " << mFiles[i].mNameHash << std::endl
                  << "  flags           " << mFiles[i].mFlags << std::endl
                  << "  nameOffset      " << mFiles[i].mNameOffset << std::endl
                  << "  relPathIndex    " << mFiles[i].mRelPathIndex << std::endl
                  << "  sectorOffset    " << mFiles[i].mSectorOffset << std::endl
                  << "  sector          " << mFiles[i].mSector << std::endl
                  << "  length          " << mFiles[i].mLength << " }" << std::endl;
    }
    std::cout << kArkDumpRule << std::endl << std::endl;
}

// NTSC-U/C: 0x0055afc8, PAL: 0x0059c1e8
void OpenArkObject::DumpStrings() const {
    std::cout << "================= OpenArkObject Strings ===================" << std::endl
              << " Files: " << std::endl;
    for (int i = 0; i < mNumFiles; ++i) {
        std::cout << "   " << mStrings + mFiles[i].mNameOffset << std::endl;
    }
    std::cout << " Paths: " << std::endl;
    for (int i = 0; i < mNumPaths; ++i) {
        std::cout << "   " << mStrings + mRelPaths[i].mPathOffset << std::endl;
    }
    std::cout << kArkDumpRule << std::endl << std::endl;
}

// NTSC-U/C: 0x0055c3a0, PAL: 0x0059d5c0
void OpenArkObject::PrintAll() const {
    PrintHeader();
    PrintFiles();
    PrintRelPaths();
    DumpStrings();
}

// NTSC-U/C: 0x0055c3e0, PAL: 0x0059d600
int IoctlFile(int nFile, int nRequest, void *pArg) {
    return sceIoctl(nFile, nRequest, pArg);
}

// NTSC-U/C: 0x0055c458, PAL: 0x0059d678
void WaitForFileIdle(int nFile) {
    if (nFile < 0) {
        return;
    }
    int nExecuting;
    do {
        sceIoctl(nFile, kSceFsExecuting, &nExecuting);
    } while (nExecuting != 0);
}

// NTSC-U/C: 0x00702650, PAL: 0x007460a0
const char *const g_apSessionArkPaths[kSessionArkCount] = {
    "ark/root.ark", "ark/levels.ark", "ark/arenas.ark"};

// NTSC-U/C: 0x004dfb20, PAL: 0x0051e280
int InitArk() {
    if (UsingArkFiles() == 0) {
        return 1;
    }
    for (int i = 0; i < kSessionArkCount; ++i) {
        if (OpenArkObject::Open(g_apSessionArkPaths[i]) == 0) {
            std::cout << " ERROR: " << __func__
                      << "() - Failed opening ark file: " << g_apSessionArkPaths[i] << "\n";
            return 0;
        }
    }
    return 1;
}

// NTSC-U/C: 0x004dfbd8, PAL: 0x0051e338
int CloseArk() {
    int nClosed = 0;
    if (UsingArkFiles()) {
        for (int i = 0; i < kSessionArkCount; ++i) {
            nClosed += OpenArkObject::Close(g_apSessionArkPaths[i]);
        }
    }
    return nClosed == 0;
}

std::vector<OpenArkObject *> g_apMountedArks;
std::vector<ArkStream> gOpenArkFileTable;
