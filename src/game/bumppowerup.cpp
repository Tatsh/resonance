#include "game/bumppowerup.h"

#include "app/hudutil.h"
#include "app/playsound.h"
#include "game/player.h"
#include "msg/bumppacket.h"
#include "msg/powerupfailedmsg.h"

int BumpPowerup::Type() {
    return kHudItemBumper;
}

int BumpPowerup::Deploy(int nTrack, int nBar, Player *pPlayer, int) {
    BumpPacket packet(pPlayer, nBar, nTrack);
    pPlayer->Send(&packet);

    if (packet.mResult != 0) {
        PlaySoundByName("SND_DEPLOY_BUMPER");
    } else {
        PowerupFailedMsg failed;
        failed.mKind = kHudItemBumper;
        failed.mPlayer = pPlayer;
        pPlayer->Send(&failed);
    }
    return packet.mResult;
}
