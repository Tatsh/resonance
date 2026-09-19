#include "rndartt/abitmap.h"

#include "os/mem.h"
#include "rndartt/apalette.h"

// 0x005eb290
void ABitmap::SetPaletteEntries(const unsigned int *pEntries, int nFirst, int nCount) {
    if (mPalette == nullptr) {
        APalette *pPalette =
            static_cast<APalette *>(AllocateTaggedMemory(sizeof(APalette), "APalette"));
        // Yes, the binary writes through the block and stores it before testing it against null.
        pPalette->mUnknown400 = 0;
        pPalette->mEnd = 0;
        mPalette = pPalette;
        if (pPalette == nullptr) {
            return;
        }
    }
    mPalette->SetEntries(pEntries, nFirst, nCount);
}
