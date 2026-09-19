#include "game/catcher.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"

namespace {

// The game mode that adds the second scheduled command.
constexpr int kGameModeWithGemCommand = 3;

// The scheduler time the two scheduling helpers read as the clock's current reading.
constexpr long long kTickNow = -1;

} // namespace

// 0x001b15a8
void Catcher::Slot4() {
    Sch::Tick tickNow{kTickNow};

    SchedulePostGemCommand(tickNow);
    if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeWithGemCommand) {
        ScheduleGemCommand(tickNow);
    }
}

// 0x001b1610
void Catcher::Slot5() {
    mClock->Withdraw(mPostGemCommand);
    if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeWithGemCommand) {
        mClock->Withdraw(mGemCommand);
    }
}

// 0x001b19a0
int Catcher::Slot6() {
    return mUnknown60 == 0;
}
