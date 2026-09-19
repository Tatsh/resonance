#include "os/arkfile.h"

#include <ctype.h>
#include <string.h>
#include <vector>

#include "os/async.h"
#include "os/cdsearch.h"
#include "os/hostmode.h"
#include "os/loadfile.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/seccache.h"

namespace {

// Chunk of the archive the header is read out of.
constexpr int kArkHeaderSector = 0;

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
    : mPath(nullptr), mTables(nullptr), mDirEntries(nullptr), mFileEntries(nullptr),
      mNames(nullptr) {
}

ArkFile::~ArkFile() {
    if (mTables != nullptr) {
        MemFreeTagged(mTables, __FILE__, __LINE__);
    }
    if (mOptimizedTable != nullptr) {
        MemFreeTagged(mOptimizedTable, __FILE__, __LINE__);
    }
}

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
    memcpy(&pArk->mHeaderUnknown0c, pRow->mBuffer, kArkHeaderSize);

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
        pArk->mHasOptimizedTable = 1;
    } else {
        pArk->mOptimizedTable = nullptr;
    }

    unsigned nTables = pArk->mTableEnd - pArk->mTableOffset;
    pArk->mTables = MemAllocTagged(nTables, __FILE__, __LINE__);
    memcpy(pArk->mTables, static_cast<char *>(pRow->mBuffer) + pArk->mTableOffset, nTables);

    // Each table's name offsets arrive relative to the archive and are rebased onto the name pool.
    pArk->mDirEntries = static_cast<ArkDirEntry *>(pArk->mTables);
    for (int i = 0; i < pArk->mDirCount; ++i) {
        pArk->mDirEntries[i].mNameOffset -= pArk->mNameOffset;
    }

    pArk->mFileEntries = reinterpret_cast<ArkFileEntry *>(
        static_cast<char *>(pArk->mTables) + pArk->mFileTableOffset - pArk->mTableOffset);
    for (int i = 0; i < pArk->mFileCount; ++i) {
        pArk->mFileEntries[i].mNameOffset -= pArk->mNameOffset;
    }

    pArk->mNames = static_cast<char *>(pArk->mTables) + pArk->mNameOffset - pArk->mTableOffset;

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

int EraseArkStream(int nHandle) {
    for (unsigned i = 0; i < g_aArkStreams.size(); ++i) {
        if (g_aArkStreams[i].mHandle == nHandle) {
            g_aArkStreams.erase(g_aArkStreams.begin() + i);
            return 0;
        }
    }
    return -1;
}
