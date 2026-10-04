#include "game/multiplierpowerup.h"

#include "app/hudutil.h"
#include "app/playsound.h"
#include "game/player.h"
#include "msg/multipliermsg.h"

namespace {

// The third word Deploy() passes to MultiplierMsg. LocalPlayer's handler does not read it.
constexpr int kRequestedFactor = 4;

// What Deploy() reports, whatever the player did with the message.
constexpr int kDeployed = 1;

} // namespace

int MultiplierPowerup::Type() {
    return kHudItemMultiplier;
}

int MultiplierPowerup::Deploy(int, int nBar, Player *pPlayer, int) {
    MultiplierMsg msg(pPlayer, nBar, kRequestedFactor);
    pPlayer->Dispatch(&msg);
    PlaySoundByName("SND_DEPLOY_MULTIPLIER");
    return kDeployed;
}
