#include "game/multiplierpowerup.h"

#include "app/hudutil.h"
#include "app/playsound.h"
#include "game/player.h"
#include "msg/multipliermsg.h"

namespace {

// The third word Deploy() passes to MultiplierMsg, whose purpose is unrecovered.
constexpr int kMultiplierUnknown0c = 4;

// What Deploy() reports, whatever the player did with the message.
constexpr int kDeployed = 1;

} // namespace

// 0x001ca210
int MultiplierPowerup::Type() {
    return kHudItemMultiplier;
}

// 0x001ca218
int MultiplierPowerup::Deploy(int, int nBar, Player *pPlayer, int) {
    MultiplierMsg msg(pPlayer, nBar, kMultiplierUnknown0c);
    pPlayer->Handle(&msg);
    PlaySoundByName("SND_DEPLOY_MULTIPLIER");
    return kDeployed;
}
