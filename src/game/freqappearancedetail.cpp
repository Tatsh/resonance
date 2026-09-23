#include "game/freqappearancedetail.h"

// 0x0024c398
void FreqAppearanceDetail::pack(FreqPart::Packed *pOut, int *pCount) {
    int nCount = 0;
    for (std::list<FreqPart *>::iterator it = mParts.begin(); it != mParts.end(); ++it) {
        (*it)->Pack(pOut);
        ++pOut;
        ++nCount;
    }
    (void)mParts.size(); // Yes, the binary counts the list again and discards the count.
    *pCount = nCount;
}
