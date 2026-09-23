#include "game/neutralizepowerup.h"

#include "app/hudutil.h"
#include "app/playsound.h"
#include "game/player.h"
#include "msg/neutralizemsg.h"
#include "msg/powerupfailedmsg.h"

// 0x001c9c38
int NeutralizePowerup::Type() {
    return kHudItemNeutralizer;
}

// 0x001c9c40
int NeutralizePowerup::Deploy(int nTrack, int nBar, Player *pPlayer, int) {
    NeutralizeMsg msg;
    msg.mUnknown04 = 0;
    msg.mBar = nBar;
    msg.mTrack = nTrack;
    msg.mPlayer = pPlayer;
    pPlayer->Send(&msg);

    if (msg.mUnknown04 != 0) {
        PlaySoundByName("SND_DEPLOY_NEUTRALIZER");
    } else {
        PowerupFailedMsg failed;
        failed.mKind = kHudItemNeutralizer;
        failed.mPlayer = pPlayer;
        pPlayer->Send(&failed);
    }
    return msg.mUnknown04;
}
