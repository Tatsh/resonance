#include "os/bootlog.h"

#ifdef ENABLE_PATCHES

#include <eeregs.h>
#include <libmc.h>
#include <sifdev.h>
#include <stdio.h>
#include <string.h>

#include "buildinfo.h"
#include "os/cycles.h"
#include "os/iop.h"

namespace {

constexpr int kCardPort = 0;
constexpr int kCardSlot = 0;
constexpr char kLogDirectory[] = "/RESONANCE";
constexpr char kLogPath[] = "/RESONANCE/BOOT.TXT";

// Sizes of the text waiting for the next write and of the largest file. A card write of a few
// kilobytes takes tens of milliseconds. The text is therefore written in batches.
constexpr int kPendingSize = 16 * 1024;
constexpr int kFileLimit = 128 * 1024;
constexpr int kLineSize = 512;

constexpr int kHeartbeatIntervalMs = 5000;
constexpr int kHeartbeatLimit = 120;

// This PMODE value turns both read circuits off. The GS then outputs BGCOLOR alone. CRTMD must be
// 1.
constexpr unsigned long long kPmodeBackgroundOnly = 1ULL << 2;
constexpr unsigned long long kFatalBackgroundRed = 0xff;

constexpr char kTruncatedMarker[] = "[boot log text dropped]\n";
constexpr char kFullMarker[] = "[boot log full]\n";

// SIF DMA moves the written text. The staging buffer therefore starts on a cache line.
alignas(64) char g_szWriteBuffer[kPendingSize];
char g_szPending[kPendingSize];
int g_nPendingLength = 0;
bool g_bPendingTruncated = false;
int g_nFileLength = 0;
bool g_bCardReady = false;
int g_nOpenedMs = 0;
int g_nLastHeartbeatMs = 0;
int g_nHeartbeats = 0;

// Waits for the command a libmc call started. A call that could not start, because the game has
// a command in flight, reports its error instead.
inline int RunCardCommand(int nIssued) {
    if (nIssued != sceMcResSucceed) {
        return nIssued;
    }
    int nResult = 0;
    sceMcSync(sceMcWait, nullptr, &nResult);
    return nResult;
}

inline void AppendText(const char *pszText, int nLength) {
    if (nLength > kPendingSize - g_nPendingLength) {
        g_bPendingTruncated = true;
        nLength = kPendingSize - g_nPendingLength;
    }
    memcpy(&g_szPending[g_nPendingLength], pszText, nLength);
    g_nPendingLength += nLength;
}

// Appends the pending text to the file. Text the card cannot take yet stays pending.
void FlushPending() {
    constexpr int kFullMarkerLength = sizeof(kFullMarker) - 1;
    constexpr int kTruncatedMarkerLength = sizeof(kTruncatedMarker) - 1;
    if (!g_bCardReady || g_nPendingLength == 0) {
        return;
    }
    if (g_nFileLength >= kFileLimit - kFullMarkerLength) {
        g_nPendingLength = 0;
        return;
    }

    int nLength = g_nPendingLength;
    memcpy(g_szWriteBuffer, g_szPending, nLength);
    if (g_bPendingTruncated && nLength + kTruncatedMarkerLength <= kPendingSize) {
        memcpy(&g_szWriteBuffer[nLength], kTruncatedMarker, kTruncatedMarkerLength);
        nLength += kTruncatedMarkerLength;
    }
    if (g_nFileLength + nLength > kFileLimit - kFullMarkerLength) {
        nLength = kFileLimit - kFullMarkerLength - g_nFileLength;
        memcpy(&g_szWriteBuffer[nLength], kFullMarker, kFullMarkerLength);
        nLength += kFullMarkerLength;
    }

    const int nFile = RunCardCommand(sceMcOpen(kCardPort, kCardSlot, kLogPath, SCE_WRONLY));
    if (nFile < 0) {
        return;
    }
    RunCardCommand(sceMcSeek(nFile, 0, SCE_SEEK_END));
    const int nWritten = RunCardCommand(sceMcWrite(nFile, g_szWriteBuffer, nLength));
    RunCardCommand(sceMcClose(nFile));
    if (nWritten > 0) {
        g_nFileLength += nWritten;
        g_nPendingLength = 0;
        g_bPendingTruncated = false;
    }
}

void AppendLineV(const char *pszFormat, va_list args) {
    if (g_nPendingLength != 0 && g_szPending[g_nPendingLength - 1] != '\n') {
        AppendText("\n", 1);
    }
    char szLine[kLineSize];
    const int nPrefix =
        snprintf(szLine, sizeof(szLine), "[%7d] ", GetElapsedMilliseconds() - g_nOpenedMs);
    int nLength = vsnprintf(&szLine[nPrefix], sizeof(szLine) - nPrefix, pszFormat, args);
    nLength = nPrefix + (nLength < static_cast<int>(sizeof(szLine)) - nPrefix ?
                             nLength :
                             static_cast<int>(sizeof(szLine)) - nPrefix - 1);
    AppendText(szLine, nLength);
    if (szLine[nLength - 1] != '\n') {
        AppendText("\n", 1);
    }
}

} // namespace

void BootLogOpen() {
    g_nOpenedMs = GetElapsedMilliseconds();
    PreloadMemoryCardModules();
    sceMcInitLibrary();

    int nType = 0;
    int nFree = 0;
    int nFormatted = 0;
    const int nInfo =
        RunCardCommand(sceMcGetInfo(kCardPort, kCardSlot, &nType, &nFree, &nFormatted));
    if ((nInfo != sceMcResSucceed && nInfo != sceMcResChangedCard) || nFormatted == 0) {
        return;
    }

    RunCardCommand(sceMcMkDir(kCardPort, kCardSlot, kLogDirectory));
    RunCardCommand(sceMcDelete(kCardPort, kCardSlot, kLogPath));
    const int nFile =
        RunCardCommand(sceMcOpen(kCardPort, kCardSlot, kLogPath, SCE_WRONLY | sceMcFileCreateFile));
    if (nFile < 0) {
        return;
    }
    RunCardCommand(sceMcClose(nFile));
    g_bCardReady = true;
    BootLogCheckpoint("resonance %s %s boot log, %d KB free on the card",
                      RESONANCE_GIT_SHA,
                      RESONANCE_BUILD_TIME,
                      nFree);
}

void BootLogCheckpoint(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    AppendLineV(pszFormat, args);
    va_end(args);
    FlushPending();
}

void BootLogAppendV(const char *pszFormat, va_list args) {
    char szText[kLineSize];
    const int nLength = vsnprintf(szText, sizeof(szText), pszFormat, args);
    AppendText(szText,
               nLength < static_cast<int>(sizeof(szText)) ? nLength :
                                                            static_cast<int>(sizeof(szText)) - 1);
}

void BootLogHeartbeat() {
    if (g_nHeartbeats >= kHeartbeatLimit) {
        return;
    }
    // A card write costs tens of milliseconds. Pending text therefore waits for the next heartbeat.
    const int nNowMs = GetElapsedMilliseconds();
    if (nNowMs - g_nLastHeartbeatMs < kHeartbeatIntervalMs) {
        return;
    }
    g_nLastHeartbeatMs = nNowMs;
    ++g_nHeartbeats;
    BootLogCheckpoint("heartbeat %d", g_nHeartbeats);
}

void BootLogShowFatalScreen() {
    *GS_PMODE = kPmodeBackgroundOnly;
    *GS_BGCOLOR = kFatalBackgroundRed;
}

#endif
