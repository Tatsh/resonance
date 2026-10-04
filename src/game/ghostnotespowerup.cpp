#include "game/ghostnotespowerup.h"

#include "app/hudutil.h"
#include "game/player.h"
#include "msg/toggleghostmsg.h"

namespace {

// ToggleGhostMsg::mOn for guides that are lit.
constexpr int kGhostOn = 1;

// Deploy() reports success every time.
constexpr int kDeployed = 1;

} // namespace

int GhostNotesPowerup::Deploy(int, int, Player *pPlayer, int) {
    ToggleGhostMsg msg;
    msg.mPlayer = pPlayer;
    msg.mOn = kGhostOn;
    pPlayer->Dispatch(&msg);
    return kDeployed;
}

int GhostNotesPowerup::Type() {
    return kHudItemGuides;
}
