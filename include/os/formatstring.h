#pragma once

/**
 * Format text into one shared buffer and return it.
 *
 * The buffer is the static array at `0x008de390`, so the result survives only until the next call
 * and no caller releases it. The routine saves eight integer and four single-precision argument
 * registers into a contiguous frame and hands that frame to `vsprintf`, which is what fixes the
 * argument list as variadic.
 *
 * @param pszFormat A printf-style format string.
 * @return The shared buffer.
 * @ghidraAddress 0x0054f688
 */
const char *FormatString(const char *pszFormat, ...);
