#include "game/inputcheatdetector.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

#include "app/application.h"
#include "app/scheduler.h"
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
constexpr int kCheatScriptTemplate = 206;

constexpr long long kNanosecondsPerMillisecond = 1000000;

// The button numbers the pad poller reports, one more than the index of the button's mask in the
// table at 0x008efb60 that 0x001e19b8 fills with the button masks in this order.
enum PadButton {
    kPadTriangle = 1,
    kPadCircle = 2,
    kPadCross = 3,
    kPadSquare = 4,
    kPadL2 = 5,
    kPadR2 = 6,
    kPadL1 = 7,
    kPadR1 = 8,
    kPadSelect = 9,
    kPadStart = 10,
    kPadR3 = 11,
    kPadL3 = 12,
    kPadUp = 13,
    kPadRight = 14,
    kPadDown = 15,
    kPadLeft = 16,
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
    // NTSC-U/C: 0x001de3c0, PAL: 0x001e43f8
    InputHistory() : mLastInputNs(0) {
    }

    long long mLastInputNs;   // +0x00
    std::vector<int> mInputs; // +0x08
};

// NTSC-U/C: 0x00691db0, PAL: 0x006d3038
// Set once RegisterCheats() has filled both tables.
int g_bCheatsRegistered;

// NTSC-U/C: 0x00691db8, PAL: 0x006d3040
InputHistory g_aInputHistories[kPlayerSlotCount];

// NTSC-U/C: 0x00891a30, PAL: 0x008d6150
// A press further apart than this from the previous one starts a new sequence.
long long g_llCheatTimeoutNs = 750000000;

} // namespace

// NTSC-U/C: 0x00691e18, PAL: 0x006d30a0
std::vector<InputCheatDetector::CheatSequence> g_metCheatSequences;

// NTSC-U/C: 0x00691e28, PAL: 0x006d30b0
std::vector<InputCheatDetector::CheatSequence> g_gameCheatSequences;

// NTSC-U/C: 0x001daaf8, PAL: 0x001e0a68
InputCheatDetector::InputCheatDetector(std::vector<CheatSequence> *pCheats) : mCheats(pCheats) {
    if (!g_bCheatsRegistered) {
        RegisterCheats();
    }
    for (InputHistory &history : g_aInputHistories) {
        history.mInputs.clear();
    }
}

// NTSC-U/C: 0x001dabc8, PAL: 0x001e0b38
void InputCheatDetector::RegisterCheats() {
    CheatSequence cheat;

    cheat.mName = "activatelistenmode";
    AppendButtons(cheat, {kPadL1, kPadR1, kPadR2, kPadSelect});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "enableteamfreqs";
    AppendButtons(cheat,
                  {kPadRight, kPadUp, kPadLeft, kPadDown, kPadRight, kPadUp, kPadLeft, kPadDown});
    AddMetCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "activatepracticemode";
    AppendButtons(cheat, {kPadUp, kPadDown, kPadLeft, kPadRight, kPadCross, kPadCross, kPadCross});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "activateallaccessmode";
    AppendButtons(cheat,
                  {kPadLeft,
                   kPadUp,
                   kPadRight,
                   kPadDown,
                   kPadLeft,
                   kPadUp,
                   kPadRight,
                   kPadDown,
                   kPadLeft,
                   kPadRight,
                   kPadLeft,
                   kPadRight,
                   kPadLeft,
                   kPadRight,
                   kPadTriangle,
                   kPadCircle,
                   kPadTriangle});
    AddMetCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "enablepowerupcheats";
    AppendButtons(cheat,
                  {kPadDown, kPadRight, kPadUp, kPadLeft, kPadLeft, kPadUp, kPadRight, kPadDown});
    AddMetCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "autocatcher";
    AppendButtons(cheat, {kPadLeft, kPadRight, kPadRight, kPadLeft, kPadUp});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "freestyle";
    AppendButtons(cheat, {kPadLeft, kPadRight, kPadRight, kPadLeft, kPadDown});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "multiplier";
    AppendButtons(cheat, {kPadRight, kPadLeft, kPadLeft, kPadRight, kPadUp});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "neutralizer";
    AppendButtons(cheat, {kPadLeft, kPadRight, kPadLeft, kPadRight, kPadUp});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "crippler";
    AppendButtons(cheat, {kPadLeft, kPadRight, kPadLeft, kPadRight, kPadDown});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "bumper";
    AppendButtons(cheat, {kPadRight, kPadLeft, kPadRight, kPadLeft, kPadUp});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "biggem";
    AppendButtons(cheat,
                  {kPadUp, kPadDown, kPadUp, kPadDown, kPadLeft, kPadRight, kPadRight, kPadLeft});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "nolattice";
    AppendButtons(cheat,
                  {kPadDown, kPadUp, kPadDown, kPadUp, kPadRight, kPadLeft, kPadLeft, kPadRight});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    cheat.mName = "lsdmode";
    AppendButtons(cheat, {kPadDown, kPadUp, kPadUp, kPadDown, kPadDown, kPadUp, kPadUp, kPadDown});
    AddGameCheat(cheat);
    cheat.mButtons.clear();

    g_bCheatsRegistered = 1;
}

// NTSC-U/C: 0x001dc658, PAL: 0x001e25e8
void InputCheatDetector::OnControllerReading(int nType, int nSlot, int nButton, float flValue) {
    if (!(nButton < kFirstNonButton) || nType != kJoystickType || !(flValue >= kPressThreshold)) {
        return;
    }

    Sch::Scheduler *pWatchdog = Application::shared()->GetWatchdog();
    const long long llNowNs =
        (GetElapsedMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;

    InputHistory &history = g_aInputHistories[nSlot - 1];
    if (!(llNowNs < history.mLastInputNs + g_llCheatTimeoutNs)) {
        history.mInputs.clear();
    }
    history.mInputs.push_back(nButton);
    history.mLastInputNs = llNowNs;

    // The binary re-reads the list and its end on every pass.
    for (auto it = mCheats->begin(); it != mCheats->end(); ++it) {
        if (std::search(history.mInputs.begin(),
                        history.mInputs.end(),
                        it->mButtons.begin(),
                        it->mButtons.end()) != history.mInputs.end()) {
            history.mInputs.clear();
            CallScriptTemplate(kCheatScriptTemplate,
                               it->mName.mStr != nullptr ? it->mName.mStr : g_szEmptyString,
                               nSlot - 1);
        }
    }
}

// NTSC-U/C: 0x001deb30, PAL: 0x001e4bb0
void InputCheatDetector::AddGameCheat(const CheatSequence &cheat) {
    g_gameCheatSequences.push_back(cheat);
}

// NTSC-U/C: 0x001deb90, PAL: 0x001e4c10
void InputCheatDetector::AddMetCheat(const CheatSequence &cheat) {
    g_metCheatSequences.push_back(cheat);
}
