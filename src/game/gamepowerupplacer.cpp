#include "game/gamepowerupplacer.h"

#include <algorithm>

#include "game/localplayer.h"
#include "mid/mbt.h"
#include "msg/displaypointermsg.h"

namespace {

// MIDI ticks in one bar of four beats, which is the unit the cursor counts in.
constexpr int kTicksPerBar = 1920;

// The task's period, and the amount this class adds before rounding down to a bar.
constexpr int kTicksPerBeat = 480;

} // namespace

// 0x001cd9b0. Both table stores and the TickTask and MsgSource teardown after them are compiler
// expansions.
GamePowerupPlacer::~GamePowerupPlacer() {
}

// 0x001cce90
void GamePowerupPlacer::OnUnknownSlot7() {
    if (mCursorBar == -1) {
        return;
    }
    DisplayPointerMsg msg(mCursorBar, mOwner->Slot4(), mOwner);
    Send(&msg);
}

// 0x001cd028. The two discarded IsFiniteMBT() calls are the shape of an assertion compiled
// without its report.
int GamePowerupPlacer::Tick(int nElapsedTicks) {
    IsFiniteMBT(kTicksPerBeat);
    const int nTick = std::min(std::max(nElapsedTicks + kTicksPerBeat, kMBTMinimum), kMBTMaximum);
    IsFiniteMBT(nTick);
    IsFiniteMBT(kTicksPerBar);
    const int nBar = nTick / kTicksPerBar;
    if (mCursorBar != -1 && mCursorBar < nBar) {
        mCursorBar = nBar;
        DisplayPointerMsg msg(mCursorBar, mOwner->Slot4(), mOwner);
        Send(&msg);
    }
    return 1;
}
