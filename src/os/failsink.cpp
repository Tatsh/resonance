#include "os/failsink.h"

#include <stdarg.h>
#include <stdio.h>

#include "os/hxstr.h"
#include "rnd/filestream.h"

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
// and Print() discards it. A sink with this handler reports nothing at all.
// 0x004ddf68
void DefaultFailReport(const char *pszMessage) {
    g_failSink.Print(pszMessage);
}

// The mode FileStream opens the log with.
constexpr int kOpenForWriting = 1;

} // namespace

inline FailSink::FailSink()
    : mReportProc(DefaultFailReport), mUnknown08(1), mDumpLevel(0), mLogStream(nullptr) {
}

inline FailSink::~FailSink() {
    delete mLogStream;
}

FailSink g_failSink;

// 0x004de0a0
void FailSink::CloseLog() {
    if (mLogStream != nullptr) {
        mLogStream->Flush();
        delete mLogStream;
    }
    mLogStream = nullptr;
}

// 0x004ddfb8
void FailSink::OpenLog(const HxStr &path) {
    CloseLog();
    mLogStream = new Rnd::FileStream(path, kOpenForWriting);
    if (mLogStream->Fail()) {
        g_failSink.Report("Couldn't open log %s",
                          path.mStr != nullptr ? path.mStr : g_szEmptyString);
        delete mLogStream;
        mLogStream = nullptr;
    }
}

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

FailSink *FailSink::Print([[maybe_unused]] const char *pszText) {
    return this;
}
