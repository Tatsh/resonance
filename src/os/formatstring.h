#pragma once

namespace Rnd {

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
