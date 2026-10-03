#include "game/phrasemaker.h"

#include "mid/tick.h"

// NTSC-U/C: 0x0019d370, PAL: 0x001a30d8
int PhraseMaker::GetPeriodOrigin() {
    return Sch::Tick(0).mTick;
}
