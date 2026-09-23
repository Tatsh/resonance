#include <stdarg.h>
#include <stdio.h>

#include "os/hxstr.h"
#include "os/log.h"
#include "script/scripttemplatemap.h"

namespace {

// The word every wrapper passes as FormatMessage()'s unread third argument.
constexpr int kFormatUnusedWord = 0;

// The stack buffer the formatted text is built in, with no bound on the format.
constexpr int kFormatMessageBufferSize = 0x1000;

} // namespace

// 0x005e4178
HxStr FormatMessage(const HxStr &format, va_list args, [[maybe_unused]] int nUnknown) {
    char szMessage[kFormatMessageBufferSize];
    vsprintf(szMessage, format.mStr != nullptr ? format.mStr : g_szEmptyString, args);
    return HxStr(szMessage);
}

// 0x005e2c80
HxStr FormatHxStr(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    HxStr message = FormatMessage(HxStr(pszFormat), args, kFormatUnusedWord);
    va_end(args);
    return message;
}

// 0x005e2d38
HxStr FormatScriptTemplate(int nTemplate, ...) {
    va_list args;
    va_start(args, nTemplate);
    HxStr message = FormatMessage(GetScriptTemplate(nTemplate), args, kFormatUnusedWord);
    va_end(args);
    return message;
}

// 0x005e4148
HxStr FormatMessage(const HxStr &format, va_list args) {
    return FormatMessage(format, args, kFormatUnusedWord);
}
