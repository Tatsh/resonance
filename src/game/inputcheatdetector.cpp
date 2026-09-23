#include "game/inputcheatdetector.h"

#include <algorithm>
#include <vector>

#include "app/application.h"
#include "app/watchdog.h"
#include "os/cycles.h"
#include "os/hxstr.h"
#include "script/scripthost.h"

namespace {

// The controllers the detector keeps a history for. Slots are numbered from 1.
constexpr int kPlayerSlotCount = 4;

// Readings of any other type, and button numbers from this one up, are ignored.
constexpr int kJoystickType = 0x6a6f7920; // 'joy '
constexpr int kFirstNonButton = 100;

// A reading below this is not a press.
constexpr double kPressThreshold = 0.1;

// The script template a matched cheat runs, with the cheat's name and the zero-based slot.
constexpr int kCheatScriptTemplate = 0xce;

constexpr long long kNanosecondsPerMillisecond = 1000000;

// One controller's recent presses.
struct InputHistory {
    // 0x001de3c0
    InputHistory() : mLastInputNs(0) {
    }

    long long mLastInputNs;   // +0x00
    std::vector<int> mInputs; // +0x08
};

// 0x00691db0. Set once RegisterCheats() has filled both tables.
int g_bCheatsRegistered;

// 0x00691db8
InputHistory g_aInputHistories[kPlayerSlotCount];

// 0x00891a30. A press further apart than this from the previous one starts a new sequence.
long long g_llCheatTimeoutNs = 750000000;

} // namespace

// 0x00691e18
std::vector<InputCheatDetector::CheatSequence> g_metCheatSequences;

// 0x00691e28
std::vector<InputCheatDetector::CheatSequence> g_gameCheatSequences;

// 0x001daaf8
InputCheatDetector::InputCheatDetector(std::vector<CheatSequence> *pCheats) : mCheats(pCheats) {
    if (!g_bCheatsRegistered) {
        RegisterCheats();
    }
    for (InputHistory &history : g_aInputHistories) {
        history.mInputs.clear();
    }
}

// 0x001dc658
void InputCheatDetector::OnUnknownSlot2(int nType, int nSlot, int nButton, float flValue) {
    if (!(nButton < kFirstNonButton) || nType != kJoystickType || !(flValue >= kPressThreshold)) {
        return;
    }

    Watchdog *pWatchdog = Application::shared()->GetWatchdog();
    const long long llNowNs =
        (GetElapsedMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;

    InputHistory &history = g_aInputHistories[nSlot - 1];
    if (!(llNowNs < history.mLastInputNs + g_llCheatTimeoutNs)) {
        history.mInputs.clear();
    }
    history.mInputs.push_back(nButton);
    history.mLastInputNs = llNowNs;

    for (CheatSequence &cheat : *mCheats) {
        if (std::search(history.mInputs.begin(),
                        history.mInputs.end(),
                        cheat.mButtons.begin(),
                        cheat.mButtons.end()) != history.mInputs.end()) {
            history.mInputs.clear();
            CallScriptTemplate(kCheatScriptTemplate,
                               cheat.mName.mStr != nullptr ? cheat.mName.mStr : g_szEmptyString,
                               nSlot - 1);
        }
    }
}
