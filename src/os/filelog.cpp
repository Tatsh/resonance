#include "os/filelog.h"

#include <ctype.h>
#include <fstream>
#include <libcdvd.h>
#include <ostream>
#include <sifdev.h>
#include <stdio.h>
#include <string.h>

#include "os/arkfile.h"
#include "os/async.h"
#include "os/fileio.h"
#include "os/hostmode.h"
#include "os/loadfile.h"

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
constexpr int kSceWriteRequest = kSceWriteOnly | kSceCreate | kSceTruncate;

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
    if (g_bFileLogOpen != 0) {
        g_fileLog << pszTrace << std::endl;
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

// 0x006ee280
std::fstream g_fileLog;

// 0x006ee320
int g_bFileLogOpen;

// 0x006ee378
char g_szFileLogPath[kFileLogPathSize];

// 0x0047c9c0
int FileOpen(const char *pszPath, int nFlags, ...) {
    const int nSceFlags = TranslateOpenFlags(nFlags);

    // The log's own file is opened without a trace line.
    char szTrace[kFileTraceSize];
    bool bQuiet = true;
    if (strcmp(pszPath, g_szFileLogPath) != 0) {
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
        nFile = LookupArkStreamForPath(pszPath);
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
        AppendPathComponent(pszPath, szPath);
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

// 0x0047ddf0
void FileLogStart(const char *pszPath) {
    strcpy(g_szFileLogPath, pszPath);
    // The image passes the default protection 0664 alongside the mode.
    g_fileLog.open(g_szFileLogPath, std::ios::out);
    g_bFileLogOpen = 1;
}

// 0x0047de48
void FileLogStop() {
    if (g_bFileLogOpen != 0) {
        g_fileLog.close();
        g_bFileLogOpen = 0;
    }
}

// 0x0047de88
void FileLogAppend(const char *pszText) {
    if (g_bFileLogOpen != 0) {
        g_fileLog << pszText << std::endl;
    }
}

// 0x0047dec0
void AppendPathComponent(const char *pszComponent, char *pszPath) {
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

// 0x0047dfb0
int FileClose(int nFile) {
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

// 0x0047e060
int FileRead(int nFile, void *pBuffer, int nLength) {
    int nRead;
    if ((nFile & kFileHandleArkStream) != 0) {
        nRead = ReadArkStreamThroughCache(nFile & ~kFileHandleArkStream, pBuffer, nLength);
    } else if ((nFile & kFileHandleSceFile) != 0) {
        AsyncCheck(1);
        sceCdSync(SCECdBlock);
        nRead = sceRead(nFile & ~kFileHandleSceFile, pBuffer, nLength);
    } else {
        nRead = LibcConsoleRead(nFile, pBuffer, nLength);
    }

    char szTrace[kFileTraceSize];
    sprintf(szTrace,
            "  read($%x,len:%d) at t:%f tdone: %f",
            nFile,
            nLength,
            kUntimedSeconds,
            kUntimedSeconds);
    TraceFileOp(szTrace);
    return nRead;
}

// 0x0047e178
int FileWrite(int nFile, const void *pBuffer, int nLength) {
    if ((nFile & kFileHandleArkStream) != 0) {
        return -1;
    }
    if ((nFile & kFileHandleSceFile) == 0) {
        return LibcConsoleWrite(nFile, pBuffer, nLength);
    }
    return sceWrite(nFile & ~kFileHandleSceFile, pBuffer, nLength);
}

// 0x0047e1c8
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

// 0x0047e2f0
int FileIsatty(int nFile) {
    if ((nFile & kFileHandleArkStream) != 0 || (nFile & kFileHandleSceFile) != 0) {
        return 0;
    }
    return LibcConsoleIsatty(nFile);
}
