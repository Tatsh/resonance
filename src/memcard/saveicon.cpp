#include "memcard/saveicon.h"

#include <stdio.h>

#include "os/hostmode.h"
#include "os/hxstr.h"

namespace {

// The icon file, relative to the data root.
constexpr char kSaveIconPath[] = "mc/freq1.icn";

// fread() reads the icon as single bytes.
constexpr size_t kSaveIconElementSize = 1;

} // namespace

// NTSC-U/C: 0x008889a8, PAL: 0x008cd0c8
unsigned char g_abSaveIcon[kSaveIconBufferSize];

// NTSC-U/C: 0x0067bfd8, PAL: 0x006bcf40
int g_nSaveIconLength;

void LoadSaveIcon() {
    FILE *pFile;
    // The binary releases the path before it tests the open.
    {
        const HxStr path = GetFreqRoot() + kSaveIconPath;
        pFile = fopen(path.mStr != nullptr ? path.mStr : g_szEmptyString, "r");
    }
    if (pFile == nullptr) {
        return;
    }
    g_nSaveIconLength = fread(g_abSaveIcon, kSaveIconElementSize, kSaveIconBufferSize, pFile);
    fclose(pFile);
}
