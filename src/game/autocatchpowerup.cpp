#include "game/autocatchpowerup.h"

#include "app/hudutil.h"
#include "app/playsound.h"
#include "game/player.h"
#include "msg/autocatchmsg.h"
#include "msg/deployedpowerupmsg.h"
#include "msg/powerupfailedmsg.h"

namespace {

// Player::Slot19() reports this for a player whose autocatch covers one bar.
constexpr int kSingleBarPlayer = 1;

// The bars the powerup covers in each case.
constexpr int kSingleBar = 1;
constexpr int kAutocatchBars = 4;

} // namespace

// 0x001c95b0
int AutocatchPowerup::Deploy(int nTrack, int nBar, Player *pPlayer, int) {
    const int nBarCount = (pPlayer->Slot19() == kSingleBarPlayer) ? kSingleBar : kAutocatchBars;
    int bCaught = 0;
    for (int i = 0; i < nBarCount; ++i) {
        AutoCatchMsg msg;
        msg.mBar = nBar + i;
        msg.mTrack = nTrack;
        msg.mPlayer = pPlayer;
        pPlayer->Send(&msg);
        bCaught = (bCaught != 0 || msg.mUnknown04 != 0);
    }

    if (bCaught != 0) {
        DeployedPowerupMsg deployed;
        deployed.mKind = kHudItemAutocatcher;
        deployed.mPlayer = pPlayer;
        deployed.mTarget = nullptr;
        deployed.mFirstBar = nBar;
        deployed.mBarCount = nBarCount;
        deployed.mTrack = nTrack;
        pPlayer->Send(&deployed);
        PlaySoundByName("SND_DEPLOY_AUTOCATCHER");
        return bCaught;
    }

    PowerupFailedMsg failed;
    failed.mKind = kHudItemAutocatcher;
    failed.mPlayer = pPlayer;
    pPlayer->Send(&failed);
    return bCaught;
}

// 0x001c95a8
int AutocatchPowerup::Type() {
    return kHudItemAutocatcher;
}
