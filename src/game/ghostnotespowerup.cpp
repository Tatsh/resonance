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

// 0x001ca0e8
int GhostNotesPowerup::Deploy(int, int, Player *pPlayer, int) {
    ToggleGhostMsg msg;
    msg.mUnknown04 = pPlayer;
    msg.mOn = kGhostOn;
    pPlayer->Handle(&msg);
    return kDeployed;
}

// 0x001ca0e0
int GhostNotesPowerup::Type() {
    return kHudItemGuides;
}
