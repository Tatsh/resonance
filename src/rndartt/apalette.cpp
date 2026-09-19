#include "rndartt/apalette.h"

#include <string.h>

// 0x00613df8
void APalette::SetEntries(const unsigned int *pEntries, int nFirst, int nCount) {
    memcpy(&mEntries[nFirst], pEntries, nCount * sizeof(unsigned int));
    mEnd = nFirst + nCount;
}
