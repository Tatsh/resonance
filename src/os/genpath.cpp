#include "os/genpath.h"

#include <stdio.h>
#include <string.h>

namespace {

// LoadBitmapFileFromPath() builds its path in a 0x100-byte stack buffer.
constexpr int kMaxPathLength = 0x100;

constexpr char kGenBitmapExtension[] = ".abm";
constexpr char kGzExtension[] = ".gz";

constexpr char kUppercaseFirst = 'A';
constexpr int kLetterCount = 26;
constexpr char kLowercaseOffset = 'a' - 'A';

} // namespace

// NTSC-U/C: 0x00725840, PAL: 0x007694e0
const char *dirpath = "gen/";

// NTSC-U/C: 0x005585a8, PAL: 0x00599700
char *BuildBitmapCacheFileName(char *pszPath, const char *pszExtension) {
    char *pszName = strrchr(pszPath, '/');
    char *pszBackslash = strrchr(pszPath, '\\');
    if (pszBackslash != nullptr && pszName < pszBackslash) {
        pszName = pszBackslash;
    }
    pszName = pszName != nullptr ? pszName + 1 : pszPath;

    const int nPrefixLength = strlen(dirpath);
    memmove(pszName + nPrefixLength, pszName, strlen(pszName) + 1);
    memcpy(pszName, dirpath, nPrefixLength);

    char *pszDot = strrchr(pszPath, '.');
    if (pszDot != nullptr) {
        // Yes, the binary starts nPrefixLength characters past the full stop.
        for (char *pch = pszDot + nPrefixLength; *pch != '\0'; ++pch) {
            if (static_cast<unsigned>(*pch - kUppercaseFirst) < kLetterCount) {
                *pch += kLowercaseOffset;
            }
        }
        *pszDot = '_';
    }
    return strcat(pszPath, pszExtension);
}

// NTSC-U/C: 0x00558538, PAL: 0x00599690
int LoadBitmapFileFromPath(const char *pszPath) {
    char szPath[kMaxPathLength];
    strcpy(szPath, pszPath);
    BuildBitmapCacheFileName(szPath, kGenBitmapExtension);
    strcat(szPath, kGzExtension);
    FILE *pFile = fopen(szPath, "rb");
    if (pFile != nullptr) {
        fclose(pFile);
    }
    return pFile != nullptr;
}

// NTSC-U/C: 0x005586b0, PAL: 0x00599808
void ReplaceFileNameExtension(char *pszPath, const char *pszExtension) {
    char *pszDot = strrchr(pszPath, '.');
    if (pszDot == nullptr) {
        strcat(pszPath, pszExtension);
        return;
    }
    const int nExtensionLength = strlen(pszExtension);
    memmove(pszDot + nExtensionLength, pszDot, strlen(pszDot) + 1);
    memcpy(pszDot, pszExtension, nExtensionLength);
}
