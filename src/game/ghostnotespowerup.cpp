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

// NTSC-U/C: 0x001ca0e8, PAL: 0x001cff88
int GhostNotesPowerup::Deploy(int, int, Player *pPlayer, int) {
    ToggleGhostMsg msg;
    msg.mPlayer = pPlayer;
    msg.mOn = kGhostOn;
    pPlayer->Handle(&msg);
    return kDeployed;
}

// NTSC-U/C: 0x001ca0e0, PAL: 0x001cff80
int GhostNotesPowerup::Type() {
    return kHudItemGuides;
}
