#include "os/log.h"

#include <iostream>
#include <stdio.h>

#include "os/hostmode.h"
#include "os/mem.h"

namespace {

// 0x0071d368. Fatal formats into this buffer with no bound, and nothing else in the image
// references the region, so no neighbour fixes its end. The size below is a placeholder rather
// than a recovered value.
constexpr int kFatalMessageSize = 1024;

// How long a warning asks to stay on screen, in whatever unit ShowScreenMessage() takes.
constexpr int kWarningMessageDuration = 50;

// How long an alert asks to stay on screen.
constexpr int kAlertMessageDuration = 50;

// The argument Warn passes as the third one to FormatMessage(). The callee never reads it.
constexpr int kFormatMessageUnknown = 1;

// 0x0071d368
char g_szFatalMessage[kFatalMessageSize];

} // namespace

void LogPrintf(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    vfprintf(stdout, pszFormat, args);
    va_end(args);
}

void Warn(const char *pszFormat, ...) {
    if (WarningsEnabled() != 1) {
        return;
    }

    va_list args;
    va_start(args, pszFormat);
    ShowReportedMessage(FormatMessage(HxStr(pszFormat), args, kFormatMessageUnknown),
                        kWarningMessageDuration);
    va_end(args);
}

void Fatal(const char *pszFormat, ...) {
    MemCloseLogAndReport();

    va_list args;
    va_start(args, pszFormat);
    vsprintf(g_szFatalMessage, pszFormat, args);
    va_end(args);

    for (;;) {
    }
}

void ShowAlertMessage(const HxStr &text) {
    ShowScreenMessage(text.mStr != nullptr ? text.mStr : g_szEmptyString, kAlertMessageDuration);
    std::cout << "Alert! " << text << std::endl;
}
