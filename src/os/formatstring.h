#pragma once

#ifdef ENABLE_PATCHES
#include <fmt/printf.h>
#endif

namespace Rnd {

#ifdef ENABLE_PATCHES
/**
 * Format typed arguments into the shared buffer, limited to 1023 bytes.
 *
 * Invalid templates are copied literally. The result remains valid until the next call.
 *
 * @param pszFormat A printf-style template, or null for an empty result.
 * @param args Typed arguments for the template.
 * @return The shared buffer.
 */
const char *FormatString(const char *pszFormat, fmt::printf_args args);

/**
 * Format typed arguments without interpreting C variadic arguments.
 *
 * @param pszFormat A printf-style template.
 * @param args Arguments checked against the template at runtime.
 * @return The shared buffer, limited to 1023 bytes.
 */
template <typename... Args>
const char *MakeString(const char *pszFormat, const Args &...args) {
    return FormatString(pszFormat, fmt::make_printf_args(args...));
}
#else
/**
 * Format text into one shared buffer and return it.
 *
 * The buffer is the static array at `0x008de390`. The result therefore remains valid only until the
 * next call, and no caller releases it. The routine saves seven integer and four
 * single-precision argument registers into a contiguous frame and hands that frame to `vsprintf`.
 * That frame is what fixes the argument list as variadic.
 *
 * @param pszFormat A printf-style format string.
 * @return The shared buffer.
 * @ghidraAddress NTSC-U/C: 0x0054f688
 * @ghidraAddress PAL: 0x0058fcc8
 */
const char *MakeString(const char *pszFormat, ...);
#endif

} // namespace Rnd

/**
 * Copy a path into one shared buffer and cut it at its last separator.
 *
 * The last slash ends the directory, or the last backslash when the path has no slash. A path with
 * neither yields an empty string. The buffer is the 0x100-byte static array at `0x008de290`, and
 * the result is valid only until the next call. The shipped program does not call it.
 *
 * @param pszPath The path.
 * @return The shared buffer, holding the directory.
 * @ghidraAddress NTSC-U/C: 0x0054f6f0
 * @ghidraAddress PAL: 0x0058fd30
 */
const char *GetDirectoryFromPath(const char *pszPath);
