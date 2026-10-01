#include "game/phrasemaker.h"

#include "mid/mbt.h"

// 0x0019d370
int PhraseMaker::GetPeriodOrigin() {
    return Mid::MBT(0).mTick;
}
