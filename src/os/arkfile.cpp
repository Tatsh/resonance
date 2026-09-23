#include "os/arkfile.h"

#include <ctype.h>
#include <iostream>
#include <libcdvd.h>
#include <sifdev.h>
#include <string.h>
#include <vector>

#include "os/async.h"
#include "os/cdsearch.h"
#include "os/fileio.h"
#include "os/hostmode.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/seccache.h"

namespace {

// Chunk of the archive the header is read out of.
constexpr int kArkHeaderSector = 0;

// ArkFile::HashName() shifts each character by one more place than the last, wrapping after 7.
constexpr int kArkHashShiftMask = 7;

// Line every dump closes with, one literal shared by all four.
constexpr char kArkDumpRule[] = "===========================================================";

// The report ArkfileGetBaseSector() writes to the log and then passes to Fatal().
constexpr char kBaseSectorMissingFormat[] =
    "ArkfileGetBaseSector: can't find arkfile with id: %d\n";

// The sceOpen() mode OpenStreamByPath() passes, which opens for reading only.
constexpr int kOpenReadOnly = 1;

// The stream table search that FindOpenArkStream() is, and that the two entry accessors expand in
// place.
inline ArkStream *FindStreamRecord(int nHandle) {
    nHandle &= ~kFileHandleArkStream;
    const int nStreams = g_aArkStreams.size();
    for (int i = 0; i < nStreams; ++i) {
        if (g_aArkStreams[i].mHandle == nHandle) {
            return &g_aArkStreams[i];
        }
    }
    return nullptr;
}

// Set once the sector cache has been brought up, so that only the first mount does it.
// 0x00725eb0
int g_bArkSectorCacheReady;

// Both disc modes build the same path, and both the host mode and the disc mode's fallback build
// the same one, so each build appears twice in the image.
void BuildDiscPath(char *pszPath, const HxStr &name) {
    strcpy(pszPath, kArkDiscRoot);
    AppendPathComponent(name.mStr != nullptr ? name.mStr : g_szEmptyString, pszPath);
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

ArkFile::ArkFile()
    : mPath(nullptr), mTables(nullptr), mFiles(nullptr), mRelPaths(nullptr), mStrings(nullptr) {
}

ArkFile::~ArkFile() {
    if (mTables != nullptr) {
        MemFreeTagged(mTables, __FILE__, __LINE__);
    }
    if (mOptimizedTable != nullptr) {
        MemFreeTagged(mOptimizedTable, __FILE__, __LINE__);
    }
}

// 0x00559858
int ArkFile::Open(const char *pszPath) {
    HxStr strName(nullptr);
    HxStr strPath(pszPath);

    if (g_bArkSectorCacheReady == 0) {
        InitSectorCache(kArkSectorCacheRows);
        g_bArkSectorCacheReady = 1;
    }
    AsyncCheck(1);

    // Every failure below returns without releasing this record, so a failed mount leaks it.
    ArkFile *pArk = new ArkFile;

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
        CdFile cdFile;
        if (sceCdSearchFile(&cdFile, strchr(szDevice, ':') + 1) == 0) {
            LogPrintf("sceCdSearchFile failed on: %s\n", szDevice);
            return 0;
        }
        pArk->mDiscLsn = cdFile.mLsn;
    }

    pArk->mOptimized = 0;
    g_apMountedArks.push_back(pArk);

    // Yes, the binary discards this lookup's result and claims a row unconditionally.
    SectorCacheFind(pArk->mFile, kArkHeaderSector);
    SectorCacheRow *pRow = SectorCacheGetLru(pArk->mFile, kArkHeaderSector);
    ReadStreamChunk(pArk->mFile, kArkHeaderSector, pRow->mBuffer, kSectorCacheRowSize);
    memcpy(pArk->mSig, pRow->mBuffer, kArkHeaderSize);

    if (pArk->mVersion != kArkVersion) {
        LogPrintf("ERROR - ARKFILE VERSION INCORRECT - PLEASE REGENERATE!\n");
        return 0;
    }

    if (pArk->mOptimized != 0) {
        LogPrintf("FOUND OPTIMIZED ARKFILE %s, optimized flag: %d\n", pszPath, pArk->mOptimized);
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
        LogPrintf("ERROR - BAD PATH IN ARKFILE HEADER FOR ARKFILE: %s\n", pszPath);
        return 0;
    }

    pszRun += sizeof(kArkRunComponent) - 1;
    strcpy(pArk->mMountPoint, *pszRun == '/' ? pszRun + 1 : pszRun);
    return 1;
}

// 0x00559f70
int ArkFile::Close(const char *pszPath) {
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
    for (unsigned i = 0; i < g_aArkStreams.size(); ++i) {
        if (g_aArkStreams[i].mFile == g_apMountedArks[nArk]->mFile) {
            EraseArkStream(g_aArkStreams[i].mHandle);
            return 0;
        }
    }

    CloseLoadFile(g_apMountedArks[nArk]->mFile);
    InvalidateCachedSectors(g_apMountedArks[nArk]->mFile);
    delete g_apMountedArks[nArk];
    g_apMountedArks.erase(g_apMountedArks.begin() + nArk);
    return 1;
}

// 0x0055a1a0
int EraseArkStream(int nHandle) {
    for (unsigned i = 0; i < g_aArkStreams.size(); ++i) {
        if (g_aArkStreams[i].mHandle == nHandle) {
            g_aArkStreams.erase(g_aArkStreams.begin() + i);
            return 0;
        }
    }
    return -1;
}

// 0x00725e88
int g_nNextArkStreamHandle = 1;

// 0x0055c158
ArkStream *FindOpenArkStream(int nHandle) {
    return FindStreamRecord(nHandle);
}

// 0x0055be80
ArkFileEntry *GetArkStreamFileEntry(int nHandle) {
    const ArkStream *pStream = FindStreamRecord(nHandle);
    return (pStream != nullptr) ? pStream->mEntry : nullptr;
}

// 0x0055bf88
int GetArkStreamInflatedSize(int nStream) {
    const ArkStream *pStream = FindStreamRecord(nStream);
    const ArkFileEntry *pEntry = (pStream != nullptr) ? pStream->mEntry : nullptr;
    if (pEntry == nullptr) {
        return -1;
    }
    return pEntry->mSize;
}

// 0x0055c000
int GetArkStreamArkId(int nHandle) {
    const ArkStream *pStream = FindOpenArkStream(nHandle);
    if (pStream == nullptr) {
        return -1;
    }
    return pStream->mFile;
}

// 0x0055c028
int GetArkStreamPosition(int nStream) {
    const ArkStream *pStream = FindOpenArkStream(nStream);
    if (pStream == nullptr) {
        return -1;
    }
    return pStream->mArkPosition;
}

// 0x0055bd38
int SeekArkStream(int nStream, int nOffset, int nOrigin) {
    ArkStream *pStream = FindOpenArkStream(nStream);
    if (pStream == nullptr) {
        return -1;
    }
    const ArkFile *pArk = pStream->FindArk();
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

// 0x0055a590
int ArkfileGetBaseSector(int nFile) {
    const unsigned nArks = g_apMountedArks.size();
    for (unsigned i = 0; i < nArks; ++i) {
        if (g_apMountedArks[i]->mFile == nFile) {
            return g_apMountedArks[i]->mDiscLsn;
        }
    }

    LogPrintf(kBaseSectorMissingFormat, nFile);
    LogPrintf("   (ark file table size: %d\n", g_apMountedArks.size());
    for (unsigned i = 0; i < g_apMountedArks.size(); ++i) {
        LogPrintf("   (ark id at index %d: %d\n", i, g_apMountedArks[i]->mFile);
    }
    Fatal(kBaseSectorMissingFormat, nFile);
    return 0;
}

// 0x0055a410
int ArkfileLogicalToPhysicalSector(int nFile, int nSector) {
    for (unsigned i = 0; i < g_apMountedArks.size(); ++i) {
        ArkFile *pArk = g_apMountedArks[i];
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
                LogPrintf("OPTIMIZED ARKFILE ORDERING FAILURE AT INDEX: %d\n", nCursor);
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

    LogPrintf("HEY!!! ArkfileLogicalToPhysicalSector can't find arkId: %d\n", nFile);
    return nSector;
}

// 0x0055c400
int OpenStreamByPath(const char *pszPath) {
    sceCdSync(SCECdBlock);
    return sceOpen(pszPath, kOpenReadOnly);
}

// 0x0055c438
void CloseLoadFile(int nFile) {
    sceClose(nFile);
}

// 0x0055c498
void ReadStreamChunk(int nFile, int nSector, void *pBuffer, unsigned nLength) {
    // Yes, the binary maps the chunk again although ReadArkStreamThroughCache() already has.
    const int nPhysical = ArkfileLogicalToPhysicalSector(nFile, nSector);
    sceCdSync(SCECdBlock);
    sceLseek(nFile, nPhysical * kSectorCacheRowSize, SCE_SEEK_SET);
    sceRead(nFile, pBuffer, nLength);
}

// 0x0055c340
short ArkFile::HashName(const char *pszName) {
    unsigned short nHash = 0;
    int nShift = 0;
    for (; *pszName != '\0'; ++pszName) {
        nHash ^= static_cast<unsigned short>(*pszName) << nShift;
        nShift = (nShift + 1) & kArkHashShiftMask;
    }
    return static_cast<short>(nHash);
}

// 0x0055c050
ArkFileEntry *ArkFile::FindFileEntry(short nNameHash,
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

// 0x0055a868
int ArkFile::MapPathToArkIndex(const char *pszPath, char *pszName, char *pszRelPath) {
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
        const ArkFile *pArk = g_apMountedArks[i];
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

// 0x0055a6d0
ArkFileEntry *
ArkFile::FindFileEntryByPath(const char *pszPath, char *pszName, char *pszRelPath, int *pnArk) {
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
    const short nNameHash = HashName(pszName);
    const short nRelPathHash = HashName(pszRelPath);

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

// 0x0055c288
int ArkFile::OpenStream(ArkFileEntry *pEntry) {
    ArkStream stream;
    stream.mArkPosition = pEntry->mSector * mSectorSize + pEntry->mSectorOffset;
    stream.mPosition = 0;
    stream.mFile = mFile;
    stream.mHandle = g_nNextArkStreamHandle++;
    stream.mEntry = pEntry;
    g_aArkStreams.push_back(stream);
    return stream.mHandle;
}

// 0x0055c1b8
ArkFile *ArkStream::FindArk() const {
    const int nArks = g_apMountedArks.size();
    for (int i = 0; i < nArks; ++i) {
        if (g_apMountedArks[i]->mFile == mFile) {
            return g_apMountedArks[i];
        }
    }
    return nullptr;
}

// 0x0055bce8
int LookupArkStreamForPath(const char *pszPath) {
    char szName[kArkNameBufferSize];
    char szRelPath[kArkRelPathBufferSize];
    int nArk;
    ArkFileEntry *pEntry = ArkFile::FindFileEntryByPath(pszPath, szName, szRelPath, &nArk);
    if (pEntry == nullptr) {
        return -1;
    }
    return g_apMountedArks[nArk]->OpenStream(pEntry);
}

// 0x0055bee0
int GetArkFileLengthByPath(const char *pszPath) {
    char szName[kArkNameBufferSize];
    char szRelPath[kArkRelPathBufferSize];
    int nArk;
    ArkFileEntry *pEntry = ArkFile::FindFileEntryByPath(pszPath, szName, szRelPath, &nArk);
    if (pEntry == nullptr) {
        return -1;
    }
    return pEntry->mLength;
}

// 0x0055a280
int ReadArkStreamThroughCache(int nHandle, void *pBuffer, unsigned nBytes) {
    ArkStream *pStream = FindOpenArkStream(nHandle);
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
            pRow = SectorCacheGetLru(nFile, nChunk);
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

// 0x0055aa38
void ArkFile::DumpHeader() const {
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

// 0x0055ac30
void ArkFile::DumpRelativePaths() const {
    std::cout << "============== OpenArkObject Relative Paths ===============" << std::endl;
    for (int i = 0; i < mNumPaths; ++i) {
        std::cout << "{ pathHash       " << mRelPaths[i].mPathHash << std::endl
                  << "  flags          " << mRelPaths[i].mFlags << std::endl
                  << "  pathOffset     " << mRelPaths[i].mPathOffset << " }" << std::endl;
    }
    std::cout << kArkDumpRule << std::endl << std::endl;
}

// 0x0055ada0
void ArkFile::DumpFiles() const {
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

// 0x0055afc8
void ArkFile::DumpStrings() const {
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

// 0x0055c3a0
void ArkFile::Dump() const {
    DumpHeader();
    DumpFiles();
    DumpRelativePaths();
    DumpStrings();
}

// 0x0055c3e0
int IoctlFile(int nFile, int nRequest, void *pArg) {
    return sceIoctl(nFile, nRequest, pArg);
}

// 0x0055c458
void WaitForFileIdle(int nFile) {
    if (nFile < 0) {
        return;
    }
    int nExecuting;
    do {
        sceIoctl(nFile, kSceFsExecuting, &nExecuting);
    } while (nExecuting != 0);
}

// 0x00702650
const char *const g_apSessionArkPaths[kSessionArkCount] = {
    "ark/root.ark", "ark/levels.ark", "ark/arenas.ark"};

// 0x004dfb20
int InitArk() {
    if (UsingArkFiles() == 0) {
        return 1;
    }
    for (int i = 0; i < kSessionArkCount; ++i) {
        if (ArkFile::Open(g_apSessionArkPaths[i]) == 0) {
            std::cout << " ERROR: InitArk() - Failed opening ark file: " << g_apSessionArkPaths[i]
                      << "\n";
            return 0;
        }
    }
    return 1;
}

// 0x004dfbd8
int CloseArk() {
    int nClosed = 0;
    if (UsingArkFiles()) {
        for (int i = 0; i < kSessionArkCount; ++i) {
            nClosed += ArkFile::Close(g_apSessionArkPaths[i]);
        }
    }
    return nClosed == 0;
}
