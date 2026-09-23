#include "game/effectpowerup.h"

#include "game/player.h"
#include "msg/jameffectmsg.h"

// 0x001c9fb8
int EffectPowerup::Type() {
    return mEffectType;
}

// 0x001c9fc0
int EffectPowerup::Deploy(int nTrack, int nBar, Player *pPlayer, int) {
    JamEffectMsg msg;
    msg.mBar = nBar;
    msg.mTrack = nTrack;
    msg.mEffect = mEffectType;
    msg.mPlayer = pPlayer;
    pPlayer->Send(&msg);
    return 1;
}
