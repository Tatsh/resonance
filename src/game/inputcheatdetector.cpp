#include "game/inputcheatdetector.h"

#include <algorithm>
#include <initializer_list>
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

// The button numbers the pad poller reports, one more than the index of the button's mask in the
// table at 0x008efb60 that 0x001e19b8 fills with the libpad masks in this order.
enum PadButton {
    kPadUp = 1,
    kPadRight = 2,
    kPadDown = 3,
    kPadLeft = 4,
    kPadSelect = 5,
    kPadL3 = 6,
    kPadR3 = 7,
    kPadStart = 8,
    kPadL2 = 9,
    kPadR1 = 10,
    kPadL1 = 11,
    kPadR2 = 12,
    kPadTriangle = 13,
    kPadCircle = 14,
    kPadCross = 15,
    kPadSquare = 16,
};

// RegisterCheats() appends every button of a sequence with its own push_back().
inline void AppendButtons(InputCheatDetector::CheatSequence &cheat,
                          std::initializer_list<int> buttons) {
    for (const int nButton : buttons) {
        cheat.mButtons.push_back(nButton);
    }
}

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

// 0x001dabc8
void InputCheatDetector::RegisterCheats() {
    CheatSequence cheat;

    cheat.mName = "activatelistenmode";
    AppendButtons(cheat, {kPadR3, kPadStart, kPadL3, kPadL2});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "enableteamfreqs";
    AppendButtons(cheat,
                  {kPadCircle,
                   kPadTriangle,
                   kPadSquare,
                   kPadCross,
                   kPadCircle,
                   kPadTriangle,
                   kPadSquare,
                   kPadCross});
    AddMetCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "activatepracticemode";
    AppendButtons(cheat,
                  {kPadTriangle, kPadCross, kPadSquare, kPadCircle, kPadDown, kPadDown, kPadDown});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "activateallaccessmode";
    AppendButtons(cheat,
                  {kPadSquare,
                   kPadTriangle,
                   kPadCircle,
                   kPadCross,
                   kPadSquare,
                   kPadTriangle,
                   kPadCircle,
                   kPadCross,
                   kPadSquare,
                   kPadCircle,
                   kPadSquare,
                   kPadCircle,
                   kPadSquare,
                   kPadCircle,
                   kPadUp,
                   kPadRight,
                   kPadUp});
    AddMetCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "enablepowerupcheats";
    AppendButtons(cheat,
                  {kPadCross,
                   kPadCircle,
                   kPadTriangle,
                   kPadSquare,
                   kPadSquare,
                   kPadTriangle,
                   kPadCircle,
                   kPadCross});
    AddMetCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "autocatcher";
    AppendButtons(cheat, {kPadSquare, kPadCircle, kPadCircle, kPadSquare, kPadTriangle});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "freestyle";
    AppendButtons(cheat, {kPadSquare, kPadCircle, kPadCircle, kPadSquare, kPadCross});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "multiplier";
    AppendButtons(cheat, {kPadCircle, kPadSquare, kPadSquare, kPadCircle, kPadTriangle});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "neutralizer";
    AppendButtons(cheat, {kPadSquare, kPadCircle, kPadSquare, kPadCircle, kPadTriangle});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "crippler";
    AppendButtons(cheat, {kPadSquare, kPadCircle, kPadSquare, kPadCircle, kPadCross});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "bumper";
    AppendButtons(cheat, {kPadCircle, kPadSquare, kPadCircle, kPadSquare, kPadTriangle});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "biggem";
    AppendButtons(cheat,
                  {kPadTriangle,
                   kPadCross,
                   kPadTriangle,
                   kPadCross,
                   kPadSquare,
                   kPadCircle,
                   kPadCircle,
                   kPadSquare});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "nolattice";
    AppendButtons(cheat,
                  {kPadCross,
                   kPadTriangle,
                   kPadCross,
                   kPadTriangle,
                   kPadCircle,
                   kPadSquare,
                   kPadSquare,
                   kPadCircle});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "lsdmode";
    AppendButtons(cheat,
                  {kPadCross,
                   kPadTriangle,
                   kPadTriangle,
                   kPadCross,
                   kPadCross,
                   kPadTriangle,
                   kPadTriangle,
                   kPadCross});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    g_bCheatsRegistered = 1;
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

// 0x001deb30
void InputCheatDetector::AddGameCheat(const CheatSequence &cheat) {
    g_gameCheatSequences.push_back(cheat);
}

// 0x001deb90
void InputCheatDetector::AddMetCheat(const CheatSequence &cheat) {
    g_metCheatSequences.push_back(cheat);
}
