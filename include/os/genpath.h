#pragma once

/**
 * Directory the platform-converted copy of a file sits in, relative to the original: "gen/".
 *
 * BuildBitmapCacheFileName() inserts it in front of a file name. The name is inferred.
 *
 * @ghidraAddress 0x00725840
 */
extern const char *g_szGenDirectory;

/**
 * Turn a path into the path of its platform-converted copy.
 *
 * g_szGenDirectory is inserted in front of the file name, after the last slash or backslash. The
 * last full stop then becomes an underscore, and pszExtension is appended. "dir/name.bmp" with
 * ".abm" becomes "dir/gen/name_bmp.abm". Uppercase letters after the full stop are lowered, but
 * the lowering starts as many characters past the full stop as g_szGenDirectory is long. A
 * three-letter extension is therefore never lowered. The name is the analysis program's.
 *
 * @param pszPath The path, rewritten in place. The buffer must take the longer result.
 * @param pszExtension The extension to append.
 * @return pszPath.
 * @ghidraAddress 0x005585a8
 */
char *BuildBitmapCacheFileName(char *pszPath, const char *pszExtension);

/**
 * Report whether a bitmap's compressed platform copy exists.
 *
 * Builds the path of the ".abm" copy, appends ".gz", and tries to open it. The file is closed at
 * once and nothing is loaded. The name is the analysis program's.
 *
 * @param pszPath The original bitmap path.
 * @return Non-zero when the ".abm.gz" file opens.
 * @ghidraAddress 0x00558538
 */
int LoadBitmapFileFromPath(const char *pszPath);

/**
 * Insert an extension in front of a path's last full stop, or append it when there is none.
 *
 * "name.bmp" with ".abm" becomes "name.abm.bmp". The name is the analysis program's.
 *
 * @param pszPath The path, rewritten in place. The buffer must take the longer result.
 * @param pszExtension The extension to insert.
 * @ghidraAddress 0x005586b0
 */
void ReplaceFileNameExtension(char *pszPath, const char *pszExtension);
