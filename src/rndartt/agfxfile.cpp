#include "rndartt/agfxfile.h"

#include <ctype.h>
#include <string.h>

#include "rndartt/abmpfile.h"
#include "rndartt/agiffile.h"
#include "rndartt/atgafile.h"

namespace {

// Extensions as ExtensionCode() packs them, first character in the low byte.
constexpr int kExtensionBmp = 0x504d42;
constexpr int kExtensionDib = 0x424944;
constexpr int kExtensionRle = 0x454c52;
constexpr int kExtensionGif = 0x464947;
constexpr int kExtensionTga = 0x414754;

constexpr int kExtensionLength = 3;
constexpr int kSecondCharShift = 8;
constexpr int kThirdCharShift = 16;
constexpr char kExtensionSeparator = '.';
constexpr char kCaseOffset = 'a' - 'A';

const char *const kReadMode = "rb";
const char *const kWriteMode = "wb";

} // namespace

// 0x005f9bc8
AGfxFile *AGfxFile::Open(const char *pszPath, int *pnError, bool bRead) {
    *pnError = kAGfxFileOk;
    FILE *pFile = fopen(pszPath, bRead ? kReadMode : kWriteMode);
    if (pFile == nullptr) {
        *pnError = kAGfxFileOpenFailed;
        return nullptr;
    }
    AGfxFile *pGfxFile = nullptr;
    switch (ExtensionCode(pszPath)) {
    case kExtensionRle:
    case kExtensionDib:
    case kExtensionBmp:
        pGfxFile = new ABmpFile(pFile);
        break;
    case kExtensionGif:
        pGfxFile = new AGifFile(pFile);
        break;
    case kExtensionTga:
        pGfxFile = new ATgaFile(pFile);
        break;
    default:
        break;
    }
    pGfxFile->mDuration = 0; // Yes, the binary writes through null for an unknown extension.
    return pGfxFile;
}

// 0x005f9d18
int AGfxFile::WriteBitmap(const char *pszPath, const ABitmap &bitmap) {
    int nError = kAGfxFileOk;
    AGfxFile *pGfxFile = Open(pszPath, &nError, false);
    if (nError == kAGfxFileOk) {
        nError = pGfxFile->Write(bitmap);
    }
    if (pGfxFile != nullptr) {
        delete pGfxFile;
    }
    return nError;
}

// 0x0062f560
int AGfxFile::ExtensionCode(const char *pszPath) {
    const char *pExtension = strrchr(pszPath, kExtensionSeparator);
    if (pExtension == nullptr) {
        return 0;
    }
    ++pExtension;
    if (*pExtension == '/' || *pExtension == '\\') {
        return 0;
    }
    unsigned char abExtension[kExtensionLength];
    memset(abExtension, 0, kExtensionLength);
    for (int i = 0; *pExtension != '\0' && i < kExtensionLength; ++i) {
        const char ch = *pExtension++;
        abExtension[i] = static_cast<unsigned char>(islower(ch) ? ch - kCaseOffset : ch);
    }
    return abExtension[0] | (abExtension[1] << kSecondCharShift) |
           (abExtension[2] << kThirdCharShift);
}
