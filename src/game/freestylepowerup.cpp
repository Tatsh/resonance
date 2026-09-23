#include "game/freestylepowerup.h"

#include "app/hudutil.h"
#include "app/playsound.h"
#include "game/player.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/enablefreestylemsg.h"
#include "msg/powerupfailedmsg.h"

namespace {

// The track and bar range a DeployedPowerupMsg carries for a powerup without one.
constexpr int kNoTrack = -1;
constexpr int kNoBar = 0;

} // namespace

// 0x001c9a28
int FreestylePowerup::Deploy(int, int nBar, Player *pPlayer, int) {
    if (pPlayer->Slot5() != 0) {
        return 0;
    }

    EnableFreestyleMsg msg;
    msg.mBar = nBar;
    msg.mPlayer = pPlayer;
    pPlayer->Send(&msg);

    if (msg.mUnknown04 != 0) {
        DeployedPowerupMsg deployed;
        deployed.mKind = kHudItemFreestyler;
        deployed.mPlayer = pPlayer;
        deployed.mTarget = nullptr;
        deployed.mFirstBar = kNoBar;
        deployed.mBarCount = kNoBar;
        deployed.mTrack = kNoTrack;
        pPlayer->Send(&deployed);
        PlaySoundByName("SND_DEPLOY_FREESTYLER");
    } else {
        PowerupFailedMsg failed;
        failed.mKind = kHudItemFreestyler;
        failed.mPlayer = pPlayer;
        pPlayer->Send(&failed);
    }
    return msg.mUnknown04;
}

// 0x001c9a20
int FreestylePowerup::Type() {
    return kHudItemFreestyler;
}
