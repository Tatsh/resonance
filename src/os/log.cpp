#include "os/log.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "os/hostmode.h"
#include "os/mem.h"
#include "script/scripttemplatemap.h"

namespace {

// Fatal formats into the buffer at 0x0071d368 with no bound, and nothing else in the image
// references the region, so no neighbour fixes its end. The size below is a placeholder rather
// than a recovered value.
constexpr int kFatalMessageSize = 1024;

// How long a warning asks to stay on screen, in whatever unit ShowScreenMessage() takes.
constexpr int kWarningMessageDuration = 50;

// How long an alert asks to stay on screen.
constexpr int kAlertMessageDuration = 50;

// The argument Warn passes as the third one to FormatMessage(). The callee never reads it.
constexpr int kFormatMessageWarnUnused = 1;

// The third argument AlertScriptTemplate() passes to FormatMessage(). The callee never reads it.
constexpr int kFormatMessageUnused = 0;

// How long an assertion report asks to stay on screen.
constexpr int kAssertionMessageDuration = 600;

// NTSC-U/C: 0x0071d368, PAL: 0x00760dd8
char g_szFatalMessage[kFatalMessageSize];

} // namespace

// NTSC-U/C: 0x0052e3e8, PAL: 0x0056da60
void Warn(const char *pszFormat, ...) {
    if (WarningsEnabled() != 1) {
        return;
    }

    va_list args;
    va_start(args, pszFormat);
    ShowReportedMessage(FormatMessage(HxStr(pszFormat), args, kFormatMessageWarnUnused),
                        kWarningMessageDuration);
    va_end(args);
}

// NTSC-U/C: 0x0052e510, PAL: 0x0056dbc0
void SysAssert(const char *pszMessage, const char *pszFile, unsigned int nLine) {
    // The image builds the text in a pre-standard strstream, terminates it with ends, and never
    // releases the frozen buffer. A string stream produces the same text without the leak.
    std::ostringstream report;
    report << "Assertion failed " << pszFile << ":" << nLine << ": " << pszMessage;
    ShowScreenMessage(report.str().c_str(), kAssertionMessageDuration);
    exit(0);
}

// NTSC-U/C: 0x0052ea68, PAL: 0x0056e160
void AlertScriptTemplate(int nTemplate, ...) {
    const HxStr format = Resid2Str(nTemplate);
    va_list args;
    va_start(args, nTemplate);
    ShowAlertMessage(FormatMessage(format, args, kFormatMessageUnused));
    va_end(args);
}

// NTSC-U/C: 0x0052e868, PAL: 0x0056df18
void Fatal(const char *pszFormat, ...) {
    MemCloseLogAndReport();

    va_list args;
    va_start(args, pszFormat);
    vsprintf(g_szFatalMessage, pszFormat, args);
    va_end(args);

    for (;;) {
    }
}

// NTSC-U/C: 0x0052e960, PAL: 0x0056e018
void Error(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    ShowAlertMessage(FormatMessage(HxStr(pszFormat), args, kFormatMessageUnused));
    va_end(args);
}

// NTSC-U/C: 0x00466368, PAL: 0x004a3d98
void ShowReportedMessage(const HxStr &text, int nDuration) {
    if (ScreenMessagesEnabled() == 1) {
        ShowScreenMessage(text.mStr != nullptr ? text.mStr : g_szEmptyString, nDuration);
    }
    std::cout << text << std::endl;
}

// NTSC-U/C: 0x004663d8, PAL: 0x004a3e08
void ShowAlertMessage(const HxStr &text) {
    ShowScreenMessage(text.mStr != nullptr ? text.mStr : g_szEmptyString, kAlertMessageDuration);
    std::cout << "Alert! " << text << std::endl;
}
