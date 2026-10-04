#include "os/formatstring.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {

// The GetDirectoryFromPath() buffer ends where the Rnd::MakeString() buffer begins.
constexpr int kDirectoryBufferSize = 0x100;

// NTSC-U/C: 0x008de290, PAL: 0x00923250
char g_szDirectoryBuffer[kDirectoryBufferSize] = {};

// The size is not recovered: nothing else in the image references the region, and the format runs
// unbounded. The value below is a placeholder rather than a recovered size.
constexpr int kFormatStringBufferSize = 1024;

// NTSC-U/C: 0x008de390, PAL: 0x00923350
char g_szFormatStringBuffer[kFormatStringBufferSize] = {};

} // namespace

const char *Rnd::MakeString(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    vsprintf(g_szFormatStringBuffer, pszFormat, args);
    va_end(args);
    return g_szFormatStringBuffer;
}

const char *GetDirectoryFromPath(const char *pszPath) {
    strcpy(g_szDirectoryBuffer, pszPath);
    char *pszSeparator = strrchr(g_szDirectoryBuffer, '/');
    if (pszSeparator == nullptr) {
        pszSeparator = strrchr(g_szDirectoryBuffer, '\\');
    }
    if (pszSeparator != nullptr) {
        *pszSeparator = '\0';
    } else {
        g_szDirectoryBuffer[0] = '\0';
    }
    return g_szDirectoryBuffer;
}
