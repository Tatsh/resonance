#include "game/gamepowerupplacer.h"

#include <algorithm>

#include "app/application.h"
#include "game/localplayer.h"
#include "game/playmap.h"
#include "game/powerupcollectioni.h"
#include "mid/mbt.h"
#include "msg/displaypointermsg.h"

namespace {

// MIDI ticks in one bar of four beats, which is the unit the cursor counts in.
constexpr int kTicksPerBar = 1920;

// The task's period, and the amount this class adds before rounding down to a bar.
constexpr int kTicksPerBeat = 480;

// The cursor bar that marks a cursor off the map.
constexpr int kNoCursor = -1;

// A cursor moved more than this many bars ahead of the current bar steps back by one.
constexpr int kMaxCursorLead = 4;

} // namespace

// 0x001ccb70
GamePowerupPlacer::GamePowerupPlacer(LocalPlayer *pOwner,
                                     Application *pApplication,
                                     PowerupCollectionI *pCollection)
    : PowerupPlacer(), TickTask(pApplication->GetSongClock(), Mid::MBT(kTicksPerBeat).mTick, 0),
      mOwner(pOwner), mApplication(pApplication), mCollection(pCollection), mCursorBar(kNoCursor) {
}

// 0x001cd9b0
// Both table stores and the TickTask and MsgSource teardown after them are compiler
// expansions.
GamePowerupPlacer::~GamePowerupPlacer() {
}

// 0x001cccb0
void GamePowerupPlacer::OnUnknownSlot6(int nStep) {
    if (nStep == 0) {
        return;
    }
    const int nMove = -nStep;
    const int nBar = mApplication->GetSongClock()->SongTick() / Mid::MBT(kTicksPerBar).mTick;
    const int nPlayerValue = mOwner->Slot4();

    if (mCursorBar == kNoCursor) {
        if (nMove != 1 || mCollection->HasSelection() == 0) {
            return;
        }
        mCursorBar = nBar;
        DisplayPointerMsg msg(nBar, nPlayerValue, mOwner);
        Send(&msg);
        return;
    }

    const int nNewBar = mCursorBar + nMove;
    mCursorBar = nNewBar;
    if (nNewBar < nBar) {
        DisplayPointerMsg remove;
        remove.mPlayerValue = kNoCursor;
        remove.mPlayer = mOwner;
        mCursorBar = kNoCursor;
        Send(&remove);
        return;
    }
    if (nBar + kMaxCursorLead < nNewBar) {
        mCursorBar = nNewBar - 1;
        return;
    }
    DisplayPointerMsg msg(nNewBar, nPlayerValue, mOwner);
    Send(&msg);
}

// 0x001cce90
void GamePowerupPlacer::OnUnknownSlot7() {
    if (mCursorBar == -1) {
        return;
    }
    DisplayPointerMsg msg(mCursorBar, mOwner->Slot4(), mOwner);
    Send(&msg);
}

// 0x001ccf30
void GamePowerupPlacer::OnUnknownSlot8() {
    if (mCursorBar == kNoCursor) {
        return;
    }
    if (mCursorBar >= Application::shared()->GetPlayMap()->Slot9()) {
        return;
    }
    mCollection->Deploy(mOwner->Slot4(), mCursorBar);

    DisplayPointerMsg remove;
    remove.mPlayerValue = kNoCursor;
    remove.mPlayer = mOwner;
    mCursorBar = kNoCursor;
    Send(&remove);
}

// 0x001cd028
int GamePowerupPlacer::Tick(int nElapsedTicks) {
    const int nBeat = Mid::MBT(kTicksPerBeat).mTick;
    const Mid::MBT tick(std::min(std::max(nElapsedTicks + nBeat, kMBTMinimum), kMBTMaximum));
    const int nBar = tick.mTick / Mid::MBT(kTicksPerBar).mTick;
    if (mCursorBar != -1 && mCursorBar < nBar) {
        mCursorBar = nBar;
        DisplayPointerMsg msg(mCursorBar, mOwner->Slot4(), mOwner);
        Send(&msg);
    }
    return 1;
}

// 0x001cde18
void GamePowerupPlacer::OnUnknownSlot4() {
    Start(kMBTInfinity);
}

// 0x001cde40
void GamePowerupPlacer::OnUnknownSlot5() {
    Stop();
}
