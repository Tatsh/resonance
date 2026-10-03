#include "game/effectpowerup.h"

#include "game/player.h"
#include "msg/jameffectmsg.h"

// NTSC-U/C: 0x001c9fb8, PAL: 0x001cfe58
int EffectPowerup::Type() {
    return mEffectType;
}

// NTSC-U/C: 0x001c9fc0, PAL: 0x001cfe60
int EffectPowerup::Deploy(int nTrack, int nBar, Player *pPlayer, int) {
    JamEffectMsg msg;
    msg.mBar = nBar;
    msg.mTrack = nTrack;
    msg.mEffect = mEffectType;
    msg.mPlayer = pPlayer;
    pPlayer->Send(&msg);
    return 1;
}
