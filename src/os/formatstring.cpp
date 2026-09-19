#include "os/formatstring.h"

#include <stdarg.h>
#include <stdio.h>

namespace {

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
