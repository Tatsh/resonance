#include "os/formatstring.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {

// GetDirectoryFromPath() buffer ends where the FormatString() buffer begins.
constexpr int kDirectoryBufferSize = 0x100;

// 0x008de290
char g_szDirectoryBuffer[kDirectoryBufferSize];

// The size is not recovered: nothing else in the image references the region, and the format runs
// unbounded. The value below is a placeholder rather than a recovered size.
constexpr int kFormatStringBufferSize = 1024;

// 0x008de390
char g_szFormatStringBuffer[kFormatStringBufferSize];

} // namespace

const char *FormatString(const char *pszFormat, ...) {
    va_list args;
    va_start(args, pszFormat);
    vsprintf(g_szFormatStringBuffer, pszFormat, args);
    va_end(args);
    return g_szFormatStringBuffer;
}

// 0x0054f6f0
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
