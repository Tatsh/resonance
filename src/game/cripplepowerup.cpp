#include "game/cripplepowerup.h"

#include "app/hudutil.h"
#include "app/playsound.h"
#include "game/player.h"
#include "msg/cripplemsg.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/powerupfailedmsg.h"

namespace {

// The track a deployed crippler reports, since it acts on the other players rather than a track.
constexpr int kNoTrack = -1;

} // namespace

// 0x001c9828
int CripplePowerup::Type() {
    return kHudItemCrippler;
}

// 0x001c9830
int CripplePowerup::Deploy(int nTrack, int nBar, Player *pPlayer, int) {
    CrippleMsg msg;
    msg.mUnknown04 = 0;
    msg.mPlayer = pPlayer;
    msg.mTrack = nTrack;
    msg.mBar = nBar;
    pPlayer->Send(&msg);

    if (msg.mUnknown04 != 0) {
        DeployedPowerupMsg deployed;
        deployed.mKind = kHudItemCrippler;
        deployed.mPlayer = pPlayer;
        deployed.mTarget = nullptr;
        deployed.mFirstBar = 0;
        deployed.mBarCount = 0;
        deployed.mTrack = kNoTrack;
        pPlayer->Send(&deployed);
        PlaySoundByName("SND_DEPLOY_CRIPPLER");
    } else {
        PowerupFailedMsg failed;
        failed.mKind = kHudItemCrippler;
        failed.mPlayer = pPlayer;
        pPlayer->Send(&failed);
    }
    return msg.mUnknown04;
}
