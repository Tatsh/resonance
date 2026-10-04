#include "game/effectpowerup.h"

#include "game/player.h"
#include "msg/jameffectmsg.h"

int EffectPowerup::Type() {
    return mEffectType;
}

int EffectPowerup::Deploy(int nTrack, int nBar, Player *pPlayer, int) {
    JamEffectMsg msg;
    msg.mBar = nBar;
    msg.mTrack = nTrack;
    msg.mEffect = mEffectType;
    msg.mPlayer = pPlayer;
    pPlayer->Send(&msg);
    return 1;
}
