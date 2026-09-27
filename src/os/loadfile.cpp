#include "os/loadfile.h"

#include <string.h>
#include <strings.h>

#include "os/arkfile.h"
#include "os/async.h"
#include "os/hostmode.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"

namespace {

// The mode every open in this file passes to FileOpen().
constexpr int kFileOpenRead = 0;

// The gzip trailer ends with the inflated size, one little-endian word.
constexpr int kGzTrailerSizeOffset = -4;

// The memory inflate path records this name as the input name.
constexpr char kMemoryInputName[] = "(in-memory)";

// The memory inflate path records this descriptor for a memory source.
constexpr int kMemoryInputDescriptor = -1;

// The memory inflate path writes this value to the initialised flag.
constexpr int kGzipInitialised = 1;

// The memory inflate path writes this value to the output enable word.
constexpr int kGzipOutputEnabled = 1;

// Allocates a destination the way both loaders do, from the selected zone when there is one.
void *AllocateLoadBuffer(unsigned nSize, const char *pszFile, int nLine) {
    if (ZoneGetCurrent() == kNoZone) {
        return MemAllocTagged(nSize, pszFile, nLine);
    }
    return ZoneAlloc(nSize);
}

} // namespace

extern "C" {

// Record the destination for the inflate output.
void GzipSetMemoryOutput(void *pDest);

// Clear the inflate state before a run.
void GzipInitState();

// Run the inflate and report the status word.
int GzipInflate(int nDescriptor);

// Report the inflate status and return zero on success.
int GzipReportInflateError();

// Report the number of bytes the inflate produced.
int GzipInflatedSize();

} // extern "C"

// 0x00761488
// The inflate input descriptor always reads minus one for a memory source.
extern int g_nGzipInputDescriptor;

// 0x00761490
// The inflate input name holds the memory name during a memory run.
extern char g_szGzipInputName[12];

// 0x00761594
// The inflate memory source appears twice for the two readers.
extern const void *g_pGzipMemorySourceCopy;

// 0x00761598
// The inflate memory source arrives here for the run.
extern const void *g_pGzipMemorySource;

// 0x0076159c
// The inflate memory source length arrives here for the run.
extern int g_nGzipMemoryLength;

// 0x00728c24
// The inflate path sets this flag once before the first run.
extern int g_nGzipInitialisedFlag;

// 0x00728c1c
// The inflate path enables this word before each run.
extern int g_nGzipOutputEnabled;

// 0x00728c10
// This word receives the inflate status.
extern int g_nGzipInflateStatus;

// 0x00555538
void *LoadWholeFile(const char *pszPath, void *pBuffer, unsigned nBufferSize, unsigned *pnSize) {
    const char *pszExtension = pszPath + strlen(pszPath) - (sizeof(kGzExtension) - 1);
    if (strcasecmp(pszExtension, kGzExtension) == 0) {
        return LoadGzFile(pszPath, pBuffer, nBufferSize, pnSize);
    }

    int nFile = FileOpen(pszPath, kFileOpenRead);
    if (nFile < 0) {
        return nullptr;
    }

    unsigned nSize = FileSeek(nFile, 0, kFileSeekEnd);
    FileSeek(nFile, 0, kFileSeekSet);

    if (pBuffer == nullptr) {
        pBuffer = AllocateLoadBuffer(nSize, __FILE__, __LINE__);
    } else if (nBufferSize < nSize) {
        FileClose(nFile);
        return nullptr;
    }

    if (pBuffer != nullptr) {
        FileRead(nFile, pBuffer, nSize);
    }
    FileClose(nFile);
    // The size is reported even when the allocation failed and nothing was read.
    *pnSize = nSize;
    return pBuffer;
}

// 0x00555678
void *LoadGzFile(const char *pszPath, void *pBuffer, unsigned nBufferSize, unsigned *pnSize) {
    int nFile = FileOpen(pszPath, kFileOpenRead);
    if (nFile < 0) {
        return nullptr;
    }

    unsigned nSize;
    if (UsingArkFiles() != 0) {
        nSize = GetArkStreamInflatedSize(nFile & ~kFileHandleArkStream);
    } else {
        nSize = GetGzFileSize(nFile);
    }

    if (pBuffer == nullptr) {
        pBuffer = AllocateLoadBuffer(nSize, __FILE__, __LINE__);
    } else if (nBufferSize < nSize) {
        FileClose(nFile);
        Fatal("  Not enough room to load: %s!\n", pszPath);
        return nullptr;
    }

    if (pBuffer != nullptr) {
        InflateGzFileWhole(nFile, pBuffer);
    }
    // The file is not closed here, unlike in LoadWholeFile().
    *pnSize = nSize;
    return pBuffer;
}

// 0x00555790
int GetStoredFileLength(const char *pszPath) {
    const int nFile = FileOpen(pszPath, kFileOpenRead);
    if (nFile < 0) {
        return 0;
    }
    const int nLength = FileSeek(nFile, 0, kFileSeekEnd);
    FileSeek(nFile, 0, kFileSeekSet);
    FileClose(nFile);
    return nLength;
}

// 0x005638c8
unsigned GetGzFileSize(int nFile) {
    FileSeek(nFile, kGzTrailerSizeOffset, kFileSeekEnd);
    unsigned nSize;
    FileRead(nFile, &nSize, sizeof(nSize));
    FileSeek(nFile, 0, kFileSeekSet);
    return nSize;
}

// 0x005636a0
// Inflate a memory gzip member, reporting a positive size on success.
int InflateGzBuffer(const void *pSource, int nSourceLength, void *pDest) {
    g_pGzipMemorySource = pSource;
    g_nGzipMemoryLength = nSourceLength;
    g_pGzipMemorySourceCopy = pSource;
    memcpy(g_szGzipInputName, kMemoryInputName, sizeof(g_szGzipInputName));
    if (g_nGzipInitialisedFlag == 0) {
        g_nGzipInitialisedFlag = kGzipInitialised;
    }
    g_nGzipInputDescriptor = kMemoryInputDescriptor;
    GzipSetMemoryOutput(pDest);
    GzipInitState();
    g_nGzipOutputEnabled = kGzipOutputEnabled;
    const int nResult = GzipInflate(g_nGzipInputDescriptor);
    g_nGzipInflateStatus = nResult;
    if (nResult < 0) {
        if (g_nGzipInputDescriptor != kMemoryInputDescriptor) {
            FileClose(g_nGzipInputDescriptor);
        }
        return -1;
    }
    if (GzipReportInflateError() != 0) {
        if (g_nGzipInputDescriptor != kMemoryInputDescriptor) {
            FileClose(g_nGzipInputDescriptor);
        }
        return -1;
    }
    if (g_nGzipInputDescriptor != kMemoryInputDescriptor) {
        FileClose(g_nGzipInputDescriptor);
    }
    return GzipInflatedSize();
}

// 0x00555800
int GetUncompressedFileLength(const char *pszPath) {
    const int nFile = FileOpen(pszPath, kFileOpenRead);
    if (nFile < 0) {
        return 0;
    }

    int nStored;
    int nSize;
    if ((nFile & kFileHandleArkStream) != 0) {
        const ArkFileEntry *pEntry = GetArkStreamFileEntry(nFile & ~kFileHandleArkStream);
        nSize = pEntry->mSize;
        nStored = pEntry->mLength;
    } else {
        nStored = FileSeek(nFile, 0, kFileSeekEnd);
        // Unlike LoadWholeFile(), the extension test here is case-sensitive.
        if (strcmp(pszPath + strlen(pszPath) - (sizeof(kGzExtension) - 1), kGzExtension) == 0) {
            FileSeek(nFile, kGzTrailerSizeOffset, kFileSeekEnd);
            FileRead(nFile, &nSize, sizeof(nSize));
        } else {
            nSize = nStored;
        }
        FileSeek(nFile, 0, kFileSeekSet);
    }
    FileClose(nFile);

    // The larger of the two is reported, so a file that compressed badly reports its stored size.
    return (nSize < nStored) ? nStored : nSize;
}
