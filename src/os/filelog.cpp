#include "os/filelog.h"

#include <ctype.h>
#include <errno.h>
#include <fstream>
#include <libcconsole.h>
#include <libcdvd.h>
#include <ostream>
#include <sifdev.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "os/async.h"
#include "os/hostmode.h"
#include "os/loadfile.h"
#include "os/openarkobject.h"

namespace {

// The newlib open() flags FileOpen() reads, and the file-service flags it translates them to.
constexpr int kNewlibAccessMask = 3;
constexpr int kNewlibReadOnly = 0;
constexpr int kNewlibWriteOnly = 1;
constexpr int kNewlibReadWrite = 2;
constexpr int kNewlibAppend = 0x8;
constexpr int kNewlibCreate = 0x200;
constexpr int kNewlibTruncate = 0x400;
constexpr int kSceReadOnly = 0x1;
constexpr int kSceWriteOnly = 0x2;
constexpr int kSceReadWrite = 0x3;
constexpr int kSceAppend = 0x100;
constexpr int kSceCreate = 0x200;
constexpr int kSceTruncate = 0x400;

// A request with any of these bits goes to the host rather than to an archive or the disc.
constexpr int kSceWriteRequest = kSceWriteOnly | kSceAppend | kSceCreate;

constexpr char kHostDevice[] = "host0:";
constexpr char kDiscDevice[] = "cdrom0:";
constexpr char kDiscPathSeparator[] = "\\";
constexpr char kDiscVersionSuffix[] = ";1";

// Bytes of the device path and of the trace line FileOpen() builds on its stack.
constexpr int kFilePathSize = 0x100;
constexpr int kFileTraceSize = 0x100;

// The trace lines carry the times of an operation, which the shipped build compiled out. Every
// time reads as zero.
constexpr float kUntimedSeconds = 0.0f;

// Writes one trace line to the file log while it is open.
inline void TraceFileOp(const char *pszTrace) {
    if (bFileLogging != 0) {
        gFileIOLog << pszTrace << std::endl;
    }
}

// Appends the handle an open produced to its trace line and writes the line.
inline void TraceOpenResult(char *pszTrace, const char *pszFormat, int nFile) {
    sprintf(pszTrace + strlen(pszTrace), pszFormat, nFile);
    TraceFileOp(pszTrace);
}

// The extensions a CD-only boot refuses to open outside an archive.
inline bool IsHostOnlyExtension(const char *pszPath) {
    const char *pszEnd = pszPath + strlen(pszPath);
    return strcmp(pszEnd - 3, ".py") == 0 || strcmp(pszEnd - 4, ".pyc") == 0 ||
           strcmp(pszEnd - 3, ".gz") == 0;
}

inline int TranslateOpenFlags(int nFlags) {
    int nSceFlags = 0;
    switch (nFlags & kNewlibAccessMask) {
    case kNewlibReadOnly:
        nSceFlags = kSceReadOnly;
        break;
    case kNewlibWriteOnly:
        nSceFlags = kSceWriteOnly;
        break;
    case kNewlibReadWrite:
        nSceFlags = kSceReadWrite;
        break;
    default:
        break;
    }
    if ((nFlags & kNewlibAppend) != 0) {
        nSceFlags |= kSceAppend;
    }
    if ((nFlags & kNewlibCreate) != 0) {
        nSceFlags |= kSceCreate;
    }
    if ((nFlags & kNewlibTruncate) != 0) {
        nSceFlags |= kSceTruncate;
    }
    return nSceFlags;
}

inline void BuildHostPath(char *pszDest, const char *pszPath) {
    strcpy(pszDest, kHostDevice);
    strcat(pszDest, pszPath);
}

} // namespace

// NTSC-U/C: 0x006ee280, PAL: 0x00731ca0
std::fstream gFileIOLog;

// NTSC-U/C: 0x006ee320, PAL: 0x00731d40
int bFileLogging = 0;

// NTSC-U/C: 0x006ee378, PAL: 0x00731d98
char logfilename[kFileLogPathSize] = {};

// NTSC-U/C: 0x0047c9c0, PAL: 0x004ba650
int FileOpen(const char *pszPath, int nFlags, ...) {
    const int nSceFlags = TranslateOpenFlags(nFlags);

    // The log's own file is opened without a trace line.
    char szTrace[kFileTraceSize];
    bool bQuiet = true;
    if (strcmp(pszPath, logfilename) != 0) {
        bQuiet = false;
        sprintf(szTrace, "open(%s) at t:%f", pszPath, kUntimedSeconds);
    }

    char szPath[kFilePathSize];
    int nFile;
    if ((nSceFlags & kSceWriteRequest) != 0) {
        if (GetHostMode() == kHostModeCdOnly && IsHostOnlyExtension(pszPath)) {
            return -1;
        }
        BuildHostPath(szPath, pszPath);
        nFile = sceOpen(szPath, nSceFlags);
        if (nFile < 0) {
            if (!bQuiet) {
                TraceOpenResult(szTrace, " bad write fd $%x", nFile);
            }
            return nFile;
        }
        if (!bQuiet) {
            TraceOpenResult(szTrace, " write fd $%x", nFile);
        }
        return nFile | kFileHandleSceFile;
    }

    if (UsingArkFiles() != 0) {
        nFile = OpenFileInArk(pszPath);
        if (nFile >= 0) {
            nFile |= kFileHandleArkStream;
            if (!bQuiet) {
                TraceOpenResult(szTrace, " ark fd $%x", nFile);
            }
            return nFile;
        }
        if (GetHostMode() == kHostModeCdOnly && IsHostOnlyExtension(pszPath)) {
            return -1;
        }
    }

    const HostMode mode = GetHostMode();
    if (mode == kHostModeCdHost || mode == kHostModeCdOnly) {
        strcpy(szPath, kDiscDevice);
        FilenameToISO9660(pszPath, szPath);
    } else if (mode == kHostModeHostOnly) {
        BuildHostPath(szPath, pszPath);
    }
    AsyncCheck(1);
    nFile = sceOpen(szPath, nSceFlags);
    if (nFile < 0) {
        sceCdSync(SCECdBlock);
        if (mode == kHostModeCdHost) {
            BuildHostPath(szPath, pszPath);
            nFile = sceOpen(szPath, nSceFlags);
        }
    }
    if (nFile < 0) {
        if (!bQuiet) {
            TraceOpenResult(szTrace, " bad fd $%x", nFile);
        }
        return nFile;
    }
    nFile |= kFileHandleSceFile;
    if (!bQuiet) {
        TraceOpenResult(szTrace, " fd $%x", nFile);
    }
    return nFile;
}

// NTSC-U/C: 0x0047ddf0, PAL: 0x004bbac8
void InitFileIOLog(char *pszPath) {
    strcpy(logfilename, pszPath);
    // The image passes the default protection 0664 alongside the mode.
    gFileIOLog.open(logfilename, std::ios::out);
    bFileLogging = 1;
}

// NTSC-U/C: 0x0047de48, PAL: 0x004bbb20
void CloseFileIOLog() {
    if (bFileLogging != 0) {
        gFileIOLog.close();
        bFileLogging = 0;
    }
}

// NTSC-U/C: 0x0047de88, PAL: 0x004bbb60
void PrintToFileIOLog(const char *pszText) {
    if (bFileLogging != 0) {
        gFileIOLog << pszText << std::endl;
    }
}

// NTSC-U/C: 0x0047dec0, PAL: 0x004bbb98
void FilenameToISO9660(const char *pszComponent, char *pszPath) {
    if (*pszComponent != '\0') {
        strcat(pszPath, kDiscPathSeparator);
    }
    char szChar[2];
    szChar[1] = '\0';
    for (const char *p = pszComponent; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            strcat(pszPath, kDiscPathSeparator);
        } else {
            szChar[0] = toupper(*p);
            strcat(pszPath, szChar);
        }
    }
    strcat(pszPath, kDiscVersionSuffix);
}

// NTSC-U/C: 0x0047dfb0, PAL: 0x004bbc88
extern "C" int close(int nFile) {
    char szTrace[kFileTraceSize];
    sprintf(szTrace, "close($%x) at t:%f", nFile, kUntimedSeconds);
    TraceFileOp(szTrace);

    if ((nFile & kFileHandleArkStream) != 0) {
        return EraseArkStream(nFile & ~kFileHandleArkStream);
    }
    if ((nFile & kFileHandleSceFile) == 0) {
        return LibcConsoleClose(nFile);
    }
    return sceClose(nFile & ~kFileHandleSceFile);
}

// NTSC-U/C: 0x0047e060, PAL: 0x004bbd38
extern "C" ssize_t read(int nFile, void *pBuffer, size_t nLength) {
    int nRead;
    if ((nFile & kFileHandleArkStream) != 0) {
        nRead = ReadArkStreamThroughCache(nFile & ~kFileHandleArkStream, pBuffer, nLength);
    } else if ((nFile & kFileHandleSceFile) != 0) {
        AsyncCheck(1);
        sceCdSync(SCECdBlock);
        nRead = sceRead(nFile & ~kFileHandleSceFile, pBuffer, nLength);
    } else {
        nRead = reax(nFile, pBuffer, nLength);
    }

    char szTrace[kFileTraceSize];
    sprintf(szTrace,
            "  read($%x,len:%d) at t:%f tdone: %f",
            nFile,
            static_cast<int>(nLength),
            kUntimedSeconds,
            kUntimedSeconds);
    TraceFileOp(szTrace);
    return nRead;
}

// NTSC-U/C: 0x0047e178, PAL: 0x004bbe50
extern "C" ssize_t write(int nFile, const void *pBuffer, size_t nLength) {
    if ((nFile & kFileHandleArkStream) != 0) {
        return -1;
    }
    if ((nFile & kFileHandleSceFile) == 0) {
        return writx(nFile, pBuffer, nLength);
    }
    return sceWrite(nFile & ~kFileHandleSceFile, pBuffer, nLength);
}

// NTSC-U/C: 0x0047e1c8, PAL: 0x004bbea0
int FileSeek(int nFile, int nOffset, int nOrigin) {
    int nPosition;
    if ((nFile & kFileHandleArkStream) != 0) {
        nPosition = SeekArkStream(nFile & ~kFileHandleArkStream, nOffset, nOrigin);
    } else if ((nFile & kFileHandleSceFile) != 0) {
        AsyncCheck(1);
        sceCdSync(SCECdBlock);
        nPosition = sceLseek(nFile & ~kFileHandleSceFile, nOffset, nOrigin);
    } else {
        nPosition = LibcConsoleLseek(nFile, nOffset, nOrigin);
    }

    char szTrace[kFileTraceSize];
    sprintf(szTrace,
            "  lseek($%x,off:%ld,type:%d) at t:%f tdone: %f",
            nFile,
            static_cast<long>(nOffset),
            nOrigin,
            kUntimedSeconds,
            kUntimedSeconds);
    TraceFileOp(szTrace);
    return nPosition;
}

// NTSC-U/C: 0x0047e2f0, PAL: 0x004bbfc8
extern "C" int isatty(int nFile) {
    if ((nFile & kFileHandleArkStream) != 0 || (nFile & kFileHandleSceFile) != 0) {
        return 0;
    }
    return LibcConsoleIsatty(nFile);
}

// The C library's system calls. The original C library called the file layer's open(), close(),
// read(), write(), lseek(), and isatty() directly, for the standard descriptors as for every file
// the game opens with the C library. This C library calls these underscored names instead.

// NTSC-U/C: 0x005da840, PAL: 0x0061c8a8
extern "C" int _open(const char *pszPath, int nFlags, ...) {
    va_list args;
    va_start(args, nFlags);
    const int nMode = va_arg(args, int);
    va_end(args);
    return FileOpen(pszPath, nFlags, nMode);
}

extern "C" int _close(int nFile) {
    return close(nFile);
}

// NTSC-U/C: 0x0062db94, PAL: 0x0066e724
extern "C" int _read(int nFile, void *pBuffer, size_t nLength) {
    return read(nFile, pBuffer, nLength);
}

extern "C" int _write(int nFile, const void *pBuffer, size_t nLength) {
    return write(nFile, pBuffer, nLength);
}

extern "C" off_t _lseek(int nFile, off_t nOffset, int nOrigin) {
    return FileSeek(nFile, static_cast<int>(nOffset), nOrigin);
}

extern "C" int _isatty(int nFile) {
    return isatty(nFile);
}

// Every descriptor reports a character device. The C library then queries isatty() before it
// buffers a stream by line, and Python's modification time lookup succeeds for an archive stream.
// NTSC-U/C: 0x00596670, PAL: 0x005d9a78
extern "C" int _fstat(int /* nFile */, struct stat *pStat) {
    memset(pStat, 0, sizeof(*pStat));
    pStat->st_mode = S_IFCHR;
    return 0;
}

// NTSC-U/C: 0x005966b8, PAL: 0x005d9ac0
extern "C" int _stat(const char * /* pszPath */, struct stat * /* pStat */) {
    errno = EIO;
    return -1;
}
