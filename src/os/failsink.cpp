#include "os/failsink.h"

#include <stdarg.h>
#include <stdio.h>

namespace {

// The buffer Format() writes to sits 0x100 bytes past the one Report() writes to, which is what
// bounds the first. Nothing bounds the second, and the size here matches its neighbour rather than
// being recovered.
constexpr int kFailMessageSize = 0x100;

// 0x00894e30
char g_szFailMessage[kFailMessageSize];

// 0x00894f30
char g_szFailFormatted[kFailMessageSize];

// The handler SetReportHandler() installs when it is given none. It hands the message to Print(),
// which discards it, so a sink left with this handler reports nothing at all.
void DefaultFailReport(const char *pszMessage) {
    g_failSink.Print(pszMessage);
}

} // namespace

FailSink g_failSink;

void FailSink::SetReportHandler(FailReportProc pfnReport) {
    mReportProc = (pfnReport != nullptr) ? pfnReport : DefaultFailReport;
}

void FailSink::Report(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    vsprintf(g_szFailMessage, pszFormat, args);
    va_end(args);

    mReportProc(g_szFailMessage);
}

FailSink *FailSink::Format(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    vsprintf(g_szFailFormatted, pszFormat, args);
    va_end(args);

    return this;
}

FailSink *FailSink::Print(const char *pszText) {
    return this;
}
