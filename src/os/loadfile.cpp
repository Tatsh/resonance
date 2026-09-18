#include "os/loadfile.h"

#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "os/hostmode.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"

// The file primitives below are the PlayStation 2 file layer, which this tree does not
// reconstruct. Their addresses are open at 0x0047c9c0, lseek at 0x0047e1c8, read at 0x0047e060,
// close at 0x0047dfb0, and strcasecmp at 0x006004d8.

namespace {

// A handle with this bit set identifies a stream inside an ark archive. The size query for such a
// stream receives the handle with the bit cleared.
constexpr int kArkStreamHandleBit = 0x4000;

// Allocates a destination the way both loaders do, from the selected zone when there is one.
void *AllocateLoadBuffer(unsigned nSize, const char *pszFile, int nLine) {
    if (ZoneGetCurrent() == kNoZone) {
        return MemAllocTagged(nSize, pszFile, nLine);
    }
    return ZoneAlloc(nSize);
}

} // namespace

void *LoadWholeFile(const char *pszPath, void *pBuffer, unsigned nBufferSize, unsigned *pnSize) {
    const char *pszExtension = pszPath + strlen(pszPath) - (sizeof(kGzExtension) - 1);
    if (strcasecmp(pszExtension, kGzExtension) == 0) {
        return LoadGzFile(pszPath, pBuffer, nBufferSize, pnSize);
    }

    int nFile = open(pszPath, 0);
    if (nFile < 0) {
        return nullptr;
    }

    unsigned nSize = lseek(nFile, 0, SEEK_END);
    lseek(nFile, 0, SEEK_SET);

    if (pBuffer == nullptr) {
        pBuffer = AllocateLoadBuffer(nSize, __FILE__, __LINE__);
    } else if (nBufferSize < nSize) {
        close(nFile);
        return nullptr;
    }

    if (pBuffer != nullptr) {
        read(nFile, pBuffer, nSize);
    }
    close(nFile);
    // The size is reported even when the allocation failed and nothing was read.
    *pnSize = nSize;
    return pBuffer;
}

void *LoadGzFile(const char *pszPath, void *pBuffer, unsigned nBufferSize, unsigned *pnSize) {
    int nFile = open(pszPath, 0);
    if (nFile < 0) {
        return nullptr;
    }

    unsigned nSize;
    if (UsingArkFiles() != 0) {
        nSize = GetArkStreamInflatedSize(nFile & ~kArkStreamHandleBit);
    } else {
        nSize = GetGzFileSize(nFile);
    }

    if (pBuffer == nullptr) {
        pBuffer = AllocateLoadBuffer(nSize, __FILE__, __LINE__);
    } else if (nBufferSize < nSize) {
        close(nFile);
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
