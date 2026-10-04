#include "game/phrasemaker.h"

#include "mid/tick.h"

int PhraseMaker::GetPeriodOrigin() {
    return Sch::Tick(0).mTick;
}
