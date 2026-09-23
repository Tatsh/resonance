#include "game/gamepowerbarmgr.h"

// 0x001c6258
int GamePowerbarMgr::GetPowerbar(int nBar) {
    return mBars[nBar].mPowerbar;
}
