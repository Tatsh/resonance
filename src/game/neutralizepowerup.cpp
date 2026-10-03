#include "game/neutralizepowerup.h"

#include "app/hudutil.h"
#include "app/playsound.h"
#include "game/player.h"
#include "msg/neutralizemsg.h"
#include "msg/powerupfailedmsg.h"

// NTSC-U/C: 0x001c9c38, PAL: 0x001cfad8
int NeutralizePowerup::Type() {
    return kHudItemNeutralizer;
}

// NTSC-U/C: 0x001c9c40, PAL: 0x001cfae0
int NeutralizePowerup::Deploy(int nTrack, int nBar, Player *pPlayer, int) {
    NeutralizeMsg msg;
    msg.mResult = 0;
    msg.mBar = nBar;
    msg.mTrack = nTrack;
    msg.mPlayer = pPlayer;
    pPlayer->Send(&msg);

    if (msg.mResult != 0) {
        PlaySoundByName("SND_DEPLOY_NEUTRALIZER");
    } else {
        PowerupFailedMsg failed;
        failed.mKind = kHudItemNeutralizer;
        failed.mPlayer = pPlayer;
        pPlayer->Send(&failed);
    }
    return msg.mResult;
}
