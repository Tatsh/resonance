#include "os/loadfile.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <zlib.h>

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

// The staging buffer holds one refill of deflate input.
constexpr int kGzipInputBufferSize = 0x2000;

// The header parser returns this method for deflate members.
constexpr int kGzipDeflated = 8;

// The deflate driver returns this status when allocation fails.
constexpr int kGzipOutOfMemory = 3;

// The two magic bytes every gzip member opens with.
constexpr char kGzipMagic[] = {'\037', '\213'};

// The name a file run records while no name is known.
constexpr char kUnknownInputName[] = "unknown";

// Reports a header whose magic does not match.
constexpr char kNotGzipFormat[] = "\n%s: not in gzip format\n";

// Reports a compression method other than deflate.
constexpr char kUnknownMethodFormat[] = "%s: unknown method %d -- get newer version of gzip\n";

// Reports flag bits the parser does not understand.
constexpr char kBadFlagsFormat[] = "%s has flags it shouldn't have for this version of gzip\n";

// Reports the input running out before the trailer.
constexpr char kUnexpectedEofFormat[] = "GZIP ERROR: %s: unexpected end of file\n";

// Prints a stream error under the input name.
constexpr char kStreamErrorFormat[] = "\n%s: %s\n";

// Reports a status word that is not deflate.
constexpr char kInvalidMethodError[] = "internal error, invalid method";

// Reports an exhausted heap from the deflate driver.
constexpr char kOutOfMemoryError[] = "gzip out of memory";

// Reports a deflate stream that fails to decode.
constexpr char kFormatViolatedError[] = "invalid compressed data--format violated";

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
int g_nGzipInputDescriptor = 0;

// 0x00761490
// The inflate input name holds the memory name during a memory run.
char g_szGzipInputName[12] = {};

// 0x00761594
// The inflate memory source appears twice for the two readers.
const void *g_pGzipMemorySourceCopy = nullptr;

// 0x00761598
// The inflate memory source arrives here for the run.
const void *g_pGzipMemorySource = nullptr;

// 0x0076159c
// The inflate memory source length arrives here for the run.
int g_nGzipMemoryLength = 0;

// 0x00728c24
// The inflate path sets this flag once before the first run.
int g_nGzipInitialisedFlag = 0;

// 0x00728c1c
// The inflate path enables this word before each run.
int g_nGzipOutputEnabled = 0;

// 0x00728c10
// The compression method, deflate until a member header or the inflate result replaces it.
int g_nGzipInflateStatus = kGzipDeflated;

// 0x00728c28
// The staging buffer refilled from the memory source or the file.
unsigned char g_bGzipInputBuffer[kGzipInputBufferSize] = {};

// 0x00761468
// The valid byte count in the staging buffer.
int g_nGzipInputLength = 0;

// 0x0076146c
// The read position in the staging buffer.
int g_nGzipInputPosition = 0;

// 0x00761470
// The inflate state word, cleared before each run.
int g_nGzipInflateState = 0;

// 0x00761478
// The cumulative input bytes fed through the staging buffer.
unsigned long long g_llGzipTotalInput = 0;

// 0x00761480
// Eight bytes the wrappers clear and only the inflate core owns otherwise.
unsigned long long g_llGzipUnknown1480 = 0;

// 0x007615a0
// The member modification time, stored once as a zero-extended word.
unsigned long long g_llGzipModificationTime = 0;

// 0x007a5b50
// The stream state word, zeroed on entry and set past the header on exit.
unsigned long long g_llGzipStreamState = 0;

// 0x007a3ea0
// The running checksum of the inflate output.
unsigned long long g_llGzipCrc = 0xffffffff;

// 0x00728c20
// A nonzero value suppresses the modification time store.
int g_nGzipTimeFlag = -1;

// 0x00728c18
// This word is set on every error path.
int g_nGzipErrorFlag = 0;

// 0x008e68d4
// The base of the inflate output.
unsigned char *g_pGzipOutputStart = nullptr;

// 0x008e68d8
// The current inflate output position, advanced by the deflate driver.
unsigned char *g_pGzipOutputCurrent = nullptr;

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

// 0x0062db60
// Prints a system error for the input name.
void GzipRoutine0062db60(const char *pszName) {
    if (pszName != nullptr && *pszName != '\0') {
        fputs(pszName, stderr);
        fputs(": ", stderr);
    }
    const char *pszError = strerror(errno);
    if (pszError != nullptr) {
        fputs(pszError, stderr);
    }
    fputc('\n', stderr);
}

// 0x00612508
// Prints a stream error under the input name and reports one.
int GzipPrintStreamError(const char *pszMessage) {
    fprintf(stderr, kStreamErrorFormat, g_szGzipInputName, pszMessage);
    g_nGzipErrorFlag = 1;
    return 1;
}

// 0x00612550
// Reports running out of input bytes and reports one.
int GzipReportUnexpectedEof() {
    if (errno != 0) {
        GzipRoutine0062db60(g_szGzipInputName);
    } else {
        fprintf(stderr, kUnexpectedEofFormat, g_szGzipInputName);
    }
    g_nGzipErrorFlag = 1;
    return 1;
}

// 0x006121a0
// Refills the staging buffer and reports its first byte.
int GzipRefillInputBuffer(int nSilentEof) {
    int nBuffered;
    if (g_nGzipInputDescriptor == kMemoryInputDescriptor) {
        const char *pSource = static_cast<const char *>(g_pGzipMemorySource);
        const char *pCopy = static_cast<const char *>(g_pGzipMemorySourceCopy);
        int nTake = g_nGzipMemoryLength - static_cast<int>(pSource - pCopy);
        if (nTake > kGzipInputBufferSize) {
            nTake = kGzipInputBufferSize;
        }
        memcpy(g_bGzipInputBuffer, pSource, static_cast<unsigned>(nTake));
        g_nGzipInputLength = nTake;
        g_pGzipMemorySource = pSource + nTake;
        nBuffered = nTake;
    } else {
        errno = 0;
        g_nGzipInputLength = 0;
        for (;;) {
            const int nHave = g_nGzipInputLength;
            const int nGot = FileRead(g_nGzipInputDescriptor,
                                      &g_bGzipInputBuffer[nHave],
                                      static_cast<unsigned>(kGzipInputBufferSize - nHave));
            if (nGot == -1 || nGot == 0) {
                break;
            }
            g_nGzipInputLength = nHave + nGot;
            if (g_nGzipInputLength >= kGzipInputBufferSize) {
                break;
            }
        }
        if (g_nGzipInputLength == 0) {
            if (nSilentEof != 0) {
                return -1;
            }
            GzipReportUnexpectedEof();
        }
        nBuffered = g_nGzipInputLength;
    }
    g_llGzipTotalInput += static_cast<unsigned>(nBuffered);
    g_nGzipInputPosition = 1;
    return g_bGzipInputBuffer[0];
}

// Reads one header byte through the staging buffer.
static int GzipGetHeaderByte() {
    if (g_nGzipInputPosition < g_nGzipInputLength) {
        return g_bGzipInputBuffer[g_nGzipInputPosition++];
    }
    return GzipRefillInputBuffer(0);
}

// 0x006123f0
// Resets the checksum with null data and updates it otherwise, reporting the checksum so far. The
// register is stored inverted, as gzip stores it, and zlib's crc32() takes and returns the plain
// value.
unsigned GzipUpdateCrc(const void *pData, int nLength) {
    unsigned nCrc;
    if (pData == nullptr) {
        nCrc = 0xffffffffU;
    } else {
        nCrc = static_cast<unsigned>(crc32(static_cast<unsigned>(g_llGzipCrc) ^ 0xffffffffU,
                                           static_cast<const Bytef *>(pData),
                                           static_cast<uInt>(nLength))) ^
               0xffffffffU;
    }
    g_llGzipCrc = nCrc;
    return nCrc ^ 0xffffffffU;
}

// 0x006125b8
void GzipSetMemoryOutput(void *pDest) {
    g_pGzipOutputStart = static_cast<unsigned char *>(pDest);
    g_pGzipOutputCurrent = static_cast<unsigned char *>(pDest);
}

// 0x00612468
void GzipInitState() {
    g_nGzipInflateState = 0;
    g_nGzipInputPosition = 0;
    g_nGzipInputLength = 0;
    g_llGzipUnknown1480 = 0;
    g_llGzipTotalInput = 0;
}

// 0x00562f88
// Parses the gzip header and reports the compression method. The descriptor is unused because the
// input always arrives through the globals.
int GzipInflate(int nDescriptor) {
    (void)nDescriptor;

    g_nGzipInflateStatus = -1;
    g_llGzipStreamState = 0;

    char bMagic[2];
    bMagic[0] = static_cast<char>(GzipGetHeaderByte());
    bMagic[1] = static_cast<char>(GzipGetHeaderByte());
    if (memcmp(bMagic, kGzipMagic, sizeof(bMagic)) != 0) {
        fprintf(stderr, kNotGzipFormat, g_szGzipInputName);
        g_nGzipErrorFlag = 1;
        return -1;
    }

    g_nGzipInflateStatus = GzipGetHeaderByte();
    if (g_nGzipInflateStatus != kGzipDeflated) {
        fprintf(stderr, kUnknownMethodFormat, g_szGzipInputName, g_nGzipInflateStatus);
        g_nGzipErrorFlag = 1;
        return -1;
    }

    const int nFlags = GzipGetHeaderByte() & 0xff;
    if ((nFlags & 0xe2) != 0) {
        fprintf(stderr, kBadFlagsFormat, g_szGzipInputName);
        g_nGzipErrorFlag = 1;
        return -1;
    }

    const unsigned nTime = static_cast<unsigned>(GzipGetHeaderByte()) |
                           (static_cast<unsigned>(GzipGetHeaderByte()) << 8) |
                           (static_cast<unsigned>(GzipGetHeaderByte()) << 16) |
                           (static_cast<unsigned>(GzipGetHeaderByte()) << 24);
    if (nTime != 0 && g_nGzipTimeFlag == 0) {
        g_llGzipModificationTime = nTime;
    }

    // The extra-field length and the operating system byte are consumed and discarded.
    GzipGetHeaderByte();
    GzipGetHeaderByte();

    if ((nFlags & 0x04) != 0) {
        int nExtra = GzipGetHeaderByte() | (GzipGetHeaderByte() << 8);
        nExtra--;
        if (nExtra != -1) {
            do {
                GzipGetHeaderByte();
                nExtra--;
            } while (nExtra != -1);
        }
    }

    if ((nFlags & 0x08) != 0) {
        while (GzipGetHeaderByte() != 0) {
        }
    }

    if ((nFlags & 0x10) != 0) {
        while (GzipGetHeaderByte() != 0) {
        }
    }

    g_llGzipStreamState = static_cast<unsigned>(g_nGzipInputPosition + 16);
    if (g_nGzipInflateStatus >= 0) {
        return g_nGzipInflateStatus;
    }
    fprintf(stderr, kNotGzipFormat, g_szGzipInputName);
    g_nGzipErrorFlag = 1;
    return -1;
}

// 0x006125d0
int GzipInflatedSize() {
    return static_cast<int>(g_pGzipOutputCurrent - g_pGzipOutputStart);
}

// 0x0063e0b0
// Runs the deflate stage and checks the trailer, reporting zero on success and three when
// allocation fails. The wrappers, globals, and contract match the image; the driver underneath
// is rebuilt over the SDK compression library rather than recovered.
int GzipInflateData() {
    z_stream stream{};
    const int nInit = inflateInit2(&stream, -MAX_WBITS);
    if (nInit == Z_MEM_ERROR) {
        return kGzipOutOfMemory;
    }
    if (nInit != Z_OK) {
        return 1;
    }
    stream.next_out = g_pGzipOutputCurrent;
    stream.avail_out = 0x7fffffffU;
    int nResult = Z_OK;
    while (nResult == Z_OK) {
        if (g_nGzipInputPosition >= g_nGzipInputLength) {
            GzipRefillInputBuffer(0);
            if (g_nGzipInputLength == 0) {
                nResult = 1;
                break;
            }
            g_nGzipInputPosition = 0;
        }
        stream.next_in = &g_bGzipInputBuffer[g_nGzipInputPosition];
        stream.avail_in = static_cast<uInt>(g_nGzipInputLength - g_nGzipInputPosition);
        nResult = inflate(&stream, Z_NO_FLUSH);
        g_nGzipInputPosition = g_nGzipInputLength - static_cast<int>(stream.avail_in);
        if (stream.next_out != g_pGzipOutputCurrent) {
            GzipUpdateCrc(g_pGzipOutputCurrent,
                          static_cast<int>(stream.next_out - g_pGzipOutputCurrent));
            g_pGzipOutputCurrent = stream.next_out;
        }
    }
    if (nResult == Z_MEM_ERROR) {
        inflateEnd(&stream);
        return kGzipOutOfMemory;
    }
    if (nResult != Z_STREAM_END) {
        inflateEnd(&stream);
        return 1;
    }
    inflateEnd(&stream);

    unsigned nStoredCrc = 0;
    for (int i = 0; i < 4; ++i) {
        const int nByte = GzipGetHeaderByte();
        if (g_nGzipInputLength == 0) {
            return 1;
        }
        nStoredCrc |= static_cast<unsigned>(nByte) << (8 * i);
    }
    unsigned nStoredSize = 0;
    for (int i = 0; i < 4; ++i) {
        const int nByte = GzipGetHeaderByte();
        if (g_nGzipInputLength == 0) {
            return 1;
        }
        nStoredSize |= static_cast<unsigned>(nByte) << (8 * i);
    }
    if (nStoredCrc != GzipUpdateCrc(g_pGzipOutputCurrent, 0)) {
        return 1;
    }
    if (nStoredSize != static_cast<unsigned>(g_pGzipOutputCurrent - g_pGzipOutputStart)) {
        return 1;
    }
    return 0;
}

// 0x0061d778
// Runs the inflate, checks the trailer, and reports the status.
int GzipReportInflateError() {
    GzipUpdateCrc(nullptr, 0);
    if (g_nGzipInflateStatus != kGzipDeflated) {
        GzipPrintStreamError(kInvalidMethodError);
        return 1;
    }
    const int nResult = GzipInflateData();
    if (nResult == kGzipOutOfMemory) {
        GzipPrintStreamError(kOutOfMemoryError);
        return 1;
    }
    if (nResult != 0) {
        GzipPrintStreamError(kFormatViolatedError);
        return 1;
    }
    return 0;
}

// 0x005635b8
// Inflates a whole file through the descriptor globals.
void InflateGzFileWhole(int nFile, void *pDest) {
    memcpy(g_szGzipInputName, kUnknownInputName, sizeof(kUnknownInputName));
    if (g_nGzipInitialisedFlag == 0) {
        g_nGzipInitialisedFlag = kGzipInitialised;
    }
    g_nGzipInputDescriptor = nFile;
    GzipSetMemoryOutput(pDest);
    GzipInitState();
    g_nGzipOutputEnabled = kGzipOutputEnabled;
    const int nResult = GzipInflate(g_nGzipInputDescriptor);
    g_nGzipInflateStatus = nResult;
    if (nResult < 0) {
        if (g_nGzipInputDescriptor != kMemoryInputDescriptor) {
            FileClose(g_nGzipInputDescriptor);
        }
        return;
    }
    if (GzipReportInflateError() != 0) {
        if (g_nGzipInputDescriptor != kMemoryInputDescriptor) {
            FileClose(g_nGzipInputDescriptor);
        }
        return;
    }
    if (g_nGzipInputDescriptor != kMemoryInputDescriptor) {
        FileClose(g_nGzipInputDescriptor);
    }
}
