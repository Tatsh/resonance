#include "game/gamepowerupplacer.h"

#include <algorithm>

#include "app/application.h"
#include "game/localplayer.h"
#include "game/playmap.h"
#include "game/powerupcollectioni.h"
#include "mid/tick.h"
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

GamePowerupPlacer::GamePowerupPlacer(LocalPlayer *pOwner,
                                     Application *pApplication,
                                     PowerupCollectionI *pCollection)
    : PowerupPlacer(), TickTask(pApplication->GetSongClock(), Sch::Tick(kTicksPerBeat).mTick, 0),
      mOwner(pOwner), mApplication(pApplication), mCollection(pCollection), mCursorBar(kNoCursor) {
}

GamePowerupPlacer::~GamePowerupPlacer() {
    // Both table stores and the TickTask and MsgSource teardown after them are compiler
    // expansions.
}

void GamePowerupPlacer::MoveCursor(int nStep) {
    if (nStep == 0) {
        return;
    }
    const int nMove = -nStep;
    const int nBar = mApplication->GetSongClock()->SongTick() / Sch::Tick(kTicksPerBar).mTick;
    const int nPlayerValue = mOwner->GetTrack();

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

void GamePowerupPlacer::AnnounceCursor() {
    if (mCursorBar == -1) {
        return;
    }
    DisplayPointerMsg msg(mCursorBar, mOwner->GetTrack(), mOwner);
    Send(&msg);
}

void GamePowerupPlacer::DeployPowerup() {
    if (mCursorBar == kNoCursor) {
        return;
    }
    if (mCursorBar >= Application::shared()->GetPlayMap()->GetEndBar()) {
        return;
    }
    mCollection->Deploy(mOwner->GetTrack(), mCursorBar);

    DisplayPointerMsg remove;
    remove.mPlayerValue = kNoCursor;
    remove.mPlayer = mOwner;
    mCursorBar = kNoCursor;
    Send(&remove);
}

int GamePowerupPlacer::Tick(int nElapsedTicks) {
    const int nBeat = Sch::Tick(kTicksPerBeat).mTick;
    const Sch::Tick tick(std::min(std::max(nElapsedTicks + nBeat, kTickMinimum), kTickMaximum));
    const int nBar = tick.mTick / Sch::Tick(kTicksPerBar).mTick;
    if (mCursorBar != -1 && mCursorBar < nBar) {
        mCursorBar = nBar;
        DisplayPointerMsg msg(mCursorBar, mOwner->GetTrack(), mOwner);
        Send(&msg);
    }
    return 1;
}

void GamePowerupPlacer::Activate() {
    Start(kTickInfinity);
}

void GamePowerupPlacer::Deactivate() {
    Stop();
}
