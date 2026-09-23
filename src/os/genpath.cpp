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

// 0x00725840
const char *g_szGenDirectory = "gen/";

// 0x005585a8
char *BuildBitmapCacheFileName(char *pszPath, const char *pszExtension) {
    char *pszName = strrchr(pszPath, '/');
    char *pszBackslash = strrchr(pszPath, '\\');
    if (pszBackslash != nullptr && pszName < pszBackslash) {
        pszName = pszBackslash;
    }
    pszName = pszName != nullptr ? pszName + 1 : pszPath;

    const int nPrefixLength = strlen(g_szGenDirectory);
    memmove(pszName + nPrefixLength, pszName, strlen(pszName) + 1);
    memcpy(pszName, g_szGenDirectory, nPrefixLength);

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

// 0x00558538
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

// 0x005586b0
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
