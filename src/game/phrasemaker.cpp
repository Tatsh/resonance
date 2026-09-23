#include "game/phrasemaker.h"

#include "mid/mbt.h"

// 0x0019d370
int PhraseMaker::Slot5() {
    const int nOrigin = 0;
    (void)IsFiniteMBT(nOrigin); // Yes, the binary discards this call's result.
    return nOrigin;
}
