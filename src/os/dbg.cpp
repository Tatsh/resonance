#include "os/dbg.h"

#include <stdarg.h>
#include <stdio.h>

#include "os/hxstr.h"
#include "rnd/filestream.h"

namespace {

// The buffer Format() writes to sits 0x100 bytes past the one Notify() writes to, which is what
// bounds the first. Nothing bounds the second, and the size here matches its neighbour rather than
// being recovered.
constexpr int kFailMessageSize = 0x100;

// NTSC-U/C: 0x00894e30, PAL: 0x008d9e40
char g_szFailMessage[kFailMessageSize] = {};

// NTSC-U/C: 0x00894f30, PAL: 0x008d9f40
char g_szFailFormatted[kFailMessageSize] = {};

// The handler SetNotify() installs when it is given none. It hands the message to Print(),
// and Print() discards it. A sink with this handler reports nothing at all.
// NTSC-U/C: 0x004ddf68, PAL: 0x0051c520
void DefaultFailReport(const char *pszMessage) {
    Rnd::TheDbg.Print(pszMessage);
}

// The mode FileStream opens the log with.
constexpr int kOpenForWriting = 1;

} // namespace

inline Rnd::Dbg::Dbg()
    : mReportProc(DefaultFailReport), mInitializedFlag(1), mDumpLevel(0), mLogStream(nullptr) {
}

inline Rnd::Dbg::~Dbg() {
    delete mLogStream;
}

Rnd::Dbg Rnd::TheDbg;

// NTSC-U/C: 0x004de0a0, PAL: 0x0051c658
void Rnd::Dbg::CloseLog() {
    if (mLogStream != nullptr) {
        mLogStream->Flush();
        delete mLogStream;
    }
    mLogStream = nullptr;
}

// NTSC-U/C: 0x004ddfb8, PAL: 0x0051c570
void Rnd::Dbg::OpenLog(const HxStr &path) {
    CloseLog();
    mLogStream = new Rnd::FileStream(path, kOpenForWriting);
    if (mLogStream->Fail()) {
        Rnd::TheDbg.Notify("Couldn't open log %s\n",
                           path.mStr != nullptr ? path.mStr : g_szEmptyString);
        delete mLogStream;
        mLogStream = nullptr;
    }
}

// NTSC-U/C: 0x004ddf90, PAL: 0x0051c548
void Rnd::Dbg::SetNotify(FailReportProc pfnReport) {
    mReportProc = (pfnReport != nullptr) ? pfnReport : DefaultFailReport;
}

// NTSC-U/C: 0x004dde28, PAL: 0x0051c3e0
void Rnd::Dbg::Notify(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    vsprintf(g_szFailMessage, pszFormat, args);
    va_end(args);

    mReportProc(g_szFailMessage);
}

// NTSC-U/C: 0x004dde98, PAL: 0x0051c450
Rnd::Dbg *Rnd::Dbg::Format(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    vsprintf(g_szFailFormatted, pszFormat, args);
    va_end(args);

    return this;
}

// NTSC-U/C: 0x004ddfb0, PAL: 0x0051c568
Rnd::Dbg *Rnd::Dbg::Print([[maybe_unused]] const char *pszText) {
    return this;
}
