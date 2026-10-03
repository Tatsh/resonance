#include "os/loadfile.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "os/async.h"
#include "os/hostmode.h"
#include "os/inflate.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/openarkobject.h"
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

// The staging buffer stores one refill of deflate input.
constexpr int kGzipInputBufferSize = 0x2000;

// The header parser returns this method for deflate members.
constexpr int kGzipDeflated = 8;

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

// The checksum register before any data, and the mask that inverts it.
constexpr unsigned long long kCrcInitial = 0xffffffff;

// The checksum consumes one byte per table step.
constexpr unsigned kCrcByteBits = 8;
constexpr unsigned kCrcIndexMask = 0xff;

// NTSC-U/C: 0x007a3ea8, PAL: 0x007e7ba8
// The reflected CRC-32 table (upstream crc_32_tab). The image stores each entry in 64 bits.
constexpr unsigned kCrc32Table[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

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

// NTSC-U/C: 0x00761488, PAL: 0x007a43b8
// The inflate input descriptor always reads minus one for a memory source.
int g_nGzipInputDescriptor = 0;

// NTSC-U/C: 0x00761490, PAL: 0x007a43c0
// The inflate input name stores the memory name during a memory run.
char g_szGzipInputName[12] = {};

// NTSC-U/C: 0x00761594, PAL: 0x007a44c4
// The inflate memory source appears twice for the two readers.
const void *g_pGzipMemorySourceCopy = nullptr;

// NTSC-U/C: 0x00761598, PAL: 0x007a44c8
// The inflate memory source arrives here for the run.
const void *g_pGzipMemorySource = nullptr;

// NTSC-U/C: 0x0076159c, PAL: 0x007a44cc
// The inflate memory source length arrives here for the run.
int g_nGzipMemoryLength = 0;

// NTSC-U/C: 0x00728c24, PAL: 0x0076bb54
// The inflate path sets this flag once before the first run.
int g_nGzipInitialisedFlag = 0;

// NTSC-U/C: 0x00728c1c, PAL: 0x0076bb4c
// The inflate path enables this word before each run.
int g_nGzipOutputEnabled = 0;

// NTSC-U/C: 0x00728c10, PAL: 0x0076bb40
// The compression method, deflate until a member header or the inflate result replaces it.
int g_nGzipInflateStatus = kGzipDeflated;

// NTSC-U/C: 0x00728c28, PAL: 0x0076bb58
// The staging buffer refilled from the memory source or the file.
unsigned char g_bGzipInputBuffer[kGzipInputBufferSize] = {};

// NTSC-U/C: 0x00761468, PAL: 0x007a4398
// The valid byte count in the staging buffer.
unsigned g_nGzipInputLength = 0;

// NTSC-U/C: 0x0076146c, PAL: 0x007a439c
// The read position in the staging buffer.
unsigned g_nGzipInputPosition = 0;

// NTSC-U/C: 0x00761470, PAL: 0x007a43a0
unsigned g_nGzipWindowPosition = 0;

// NTSC-U/C: 0x00731468, PAL: 0x00774398
unsigned char g_bGzipWindow[2 * kGzipWindowSize] = {};

// NTSC-U/C: 0x00761478, PAL: 0x007a43a8
// The cumulative input bytes fed through the staging buffer.
unsigned long long g_llGzipTotalInput = 0;

// NTSC-U/C: 0x00761480, PAL: 0x007a43b0
// The cumulative output bytes flushed from the window.
unsigned long long g_llGzipTotalOutput = 0;

// NTSC-U/C: 0x007615a0, PAL: 0x007a44d0
// The member modification time, stored once as a zero-extended word.
unsigned long long g_llGzipModificationTime = 0;

// NTSC-U/C: 0x007a5b50, PAL: 0x007e9850
// The stream state word, zeroed on entry and set past the header on exit.
unsigned long long g_llGzipStreamState = 0;

// NTSC-U/C: 0x007a3ea0, PAL: 0x007e7ba0
// The running checksum of the inflate output.
unsigned long long g_llGzipCrc = 0xffffffff;

// NTSC-U/C: 0x00728c20, PAL: 0x0076bb50
// A nonzero value suppresses the modification time store.
int g_nGzipTimeFlag = -1;

// NTSC-U/C: 0x00728c18, PAL: 0x0076bb48
// This word is set on every error path.
int g_nGzipErrorFlag = 0;

// NTSC-U/C: 0x008e68d4, PAL: 0x0092b8d4
// The base of the inflate output.
unsigned char *g_pGzipOutputStart = nullptr;

// NTSC-U/C: 0x008e68d8, PAL: 0x0092b8d8
// The current inflate output position, advanced by the deflate driver.
unsigned char *g_pGzipOutputCurrent = nullptr;

// NTSC-U/C: 0x00555538, PAL: 0x00595bc0
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

// NTSC-U/C: 0x00555678, PAL: 0x00595d00
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

// NTSC-U/C: 0x00555790, PAL: 0x00595e18
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

// NTSC-U/C: 0x005638c8, PAL: 0x005a2038
unsigned GetGzFileSize(int nFile) {
    FileSeek(nFile, kGzTrailerSizeOffset, kFileSeekEnd);
    unsigned nSize;
    FileRead(nFile, &nSize, sizeof(nSize));
    FileSeek(nFile, 0, kFileSeekSet);
    return nSize;
}

// NTSC-U/C: 0x005636a0, PAL: 0x005a1e10
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

// NTSC-U/C: 0x00555800, PAL: 0x00595e88
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

    // The larger of the two is reported. A file that compressed badly reports its stored size.
    return (nSize < nStored) ? nStored : nSize;
}

// NTSC-U/C: 0x0062db60, PAL: 0x0066e6f0
// Prints a system error for the input name.
void GzipPrintSystemError(const char *pszName) {
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

// NTSC-U/C: 0x00612508, PAL: 0x00653098
// Prints a stream error under the input name and reports one.
int GzipPrintStreamError(const char *pszMessage) {
    fprintf(stderr, kStreamErrorFormat, g_szGzipInputName, pszMessage);
    g_nGzipErrorFlag = 1;
    return 1;
}

// NTSC-U/C: 0x00612550, PAL: 0x006530e0
// Reports running out of input bytes and reports one.
int GzipReportUnexpectedEof() {
    if (errno != 0) {
        GzipPrintSystemError(g_szGzipInputName);
    } else {
        fprintf(stderr, kUnexpectedEofFormat, g_szGzipInputName);
    }
    g_nGzipErrorFlag = 1;
    return 1;
}

// NTSC-U/C: 0x006121a0, PAL: 0x00652d30
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

// NTSC-U/C: 0x006123f0, PAL: 0x00652f80
// Resets the checksum with null data and updates it otherwise, reporting the checksum so far. The
// register is stored inverted.
unsigned long long GzipUpdateCrc(const unsigned char *pData, unsigned nLength) {
    unsigned long long llCrc;
    if (pData == nullptr) {
        llCrc = kCrcInitial;
    } else {
        llCrc = g_llGzipCrc;
        if (nLength != 0) {
            do {
                llCrc = kCrc32Table[(static_cast<unsigned>(llCrc) ^ *pData++) & kCrcIndexMask] ^
                        (llCrc >> kCrcByteBits);
            } while (--nLength);
        }
    }
    g_llGzipCrc = llCrc;
    return llCrc ^ kCrcInitial;
}

// NTSC-U/C: 0x006122f8, PAL: 0x00652e88
void GzipFlushWindow() {
    if (g_nGzipWindowPosition == 0) {
        return;
    }
    GzipUpdateCrc(g_bGzipWindow, g_nGzipWindowPosition);
    memcpy(g_pGzipOutputCurrent, g_bGzipWindow, g_nGzipWindowPosition);
    g_pGzipOutputCurrent += g_nGzipWindowPosition;
    g_llGzipTotalOutput += g_nGzipWindowPosition;
    g_nGzipWindowPosition = 0;
}

// NTSC-U/C: 0x006125b8, PAL: 0x00653148
void GzipSetMemoryOutput(void *pDest) {
    g_pGzipOutputStart = static_cast<unsigned char *>(pDest);
    g_pGzipOutputCurrent = static_cast<unsigned char *>(pDest);
}

// NTSC-U/C: 0x00612468, PAL: 0x00652ff8
void GzipInitState() {
    g_nGzipWindowPosition = 0;
    g_nGzipInputPosition = 0;
    g_nGzipInputLength = 0;
    g_llGzipTotalOutput = 0;
    g_llGzipTotalInput = 0;
}

// NTSC-U/C: 0x00562f88, PAL: 0x005a16f8
// Parses the gzip header and reports the compression method. The descriptor is unused because the
// input always arrives through the globals.
int GzipInflate(int nDescriptor) {
    (void)nDescriptor;

    g_nGzipInflateStatus = -1;
    g_llGzipStreamState = 0;

    char bMagic[2];
    bMagic[0] = static_cast<char>(GzipGetByte());
    bMagic[1] = static_cast<char>(GzipGetByte());
    if (memcmp(bMagic, kGzipMagic, sizeof(bMagic)) != 0) {
        fprintf(stderr, kNotGzipFormat, g_szGzipInputName);
        g_nGzipErrorFlag = 1;
        return -1;
    }

    g_nGzipInflateStatus = GzipGetByte();
    if (g_nGzipInflateStatus != kGzipDeflated) {
        fprintf(stderr, kUnknownMethodFormat, g_szGzipInputName, g_nGzipInflateStatus);
        g_nGzipErrorFlag = 1;
        return -1;
    }

    const int nFlags = GzipGetByte() & 0xff;
    if ((nFlags & 0xe2) != 0) {
        fprintf(stderr, kBadFlagsFormat, g_szGzipInputName);
        g_nGzipErrorFlag = 1;
        return -1;
    }

    const unsigned nTime =
        static_cast<unsigned>(GzipGetByte()) | (static_cast<unsigned>(GzipGetByte()) << 8) |
        (static_cast<unsigned>(GzipGetByte()) << 16) | (static_cast<unsigned>(GzipGetByte()) << 24);
    if (nTime != 0 && g_nGzipTimeFlag == 0) {
        g_llGzipModificationTime = nTime;
    }

    // The extra-field length and the operating system byte are consumed and discarded.
    GzipGetByte();
    GzipGetByte();

    if ((nFlags & 0x04) != 0) {
        int nExtra = GzipGetByte() | (GzipGetByte() << 8);
        nExtra--;
        if (nExtra != -1) {
            do {
                GzipGetByte();
                nExtra--;
            } while (nExtra != -1);
        }
    }

    if ((nFlags & 0x08) != 0) {
        while (GzipGetByte() != 0) {
        }
    }

    if ((nFlags & 0x10) != 0) {
        while (GzipGetByte() != 0) {
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

// NTSC-U/C: 0x006125d0, PAL: 0x00653160
int GzipInflatedSize() {
    return static_cast<int>(g_pGzipOutputCurrent - g_pGzipOutputStart);
}

// NTSC-U/C: 0x0061d778, PAL: 0x0065e308
// Runs the inflate and reports the status. The trailer checksum and length are not checked.
int GzipReportInflateError() {
    GzipUpdateCrc(nullptr, 0);
    if (g_nGzipInflateStatus != kGzipDeflated) {
        GzipPrintStreamError(kInvalidMethodError);
        return 1;
    }
    const int nResult = inflate();
    if (nResult == kInflateOutOfMemory) {
        GzipPrintStreamError(kOutOfMemoryError);
        return 1;
    }
    if (nResult != 0) {
        GzipPrintStreamError(kFormatViolatedError);
        return 1;
    }
    return 0;
}

// NTSC-U/C: 0x005635b8, PAL: 0x005a1d28
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
