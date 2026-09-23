#include <libmtap.h>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/grooveworld.h"
#include "game/inputmap.h"
#include "game/inputpoller.h"
#include "game/joypad.h"
#include "game/rawcontroller.h"
#include "msg/pausegamesystemmsg.h"
#include "os/hostmode.h"
#include "sch/command.h"
#include "script/scripthost.h"

namespace {

// The pad library's button bits, one per control.
constexpr unsigned int kPadButtonSelect = 0x0001;
constexpr unsigned int kPadButtonL3 = 0x0002;
constexpr unsigned int kPadButtonR3 = 0x0004;
constexpr unsigned int kPadButtonStart = 0x0008;
constexpr unsigned int kPadButtonUp = 0x0010;
constexpr unsigned int kPadButtonRight = 0x0020;
constexpr unsigned int kPadButtonDown = 0x0040;
constexpr unsigned int kPadButtonLeft = 0x0080;
constexpr unsigned int kPadButtonL2 = 0x0100;
constexpr unsigned int kPadButtonR2 = 0x0200;
constexpr unsigned int kPadButtonL1 = 0x0400;
constexpr unsigned int kPadButtonR1 = 0x0800;
constexpr unsigned int kPadButtonTriangle = 0x1000;
constexpr unsigned int kPadButtonCircle = 0x2000;
constexpr unsigned int kPadButtonCross = 0x4000;
constexpr unsigned int kPadButtonSquare = 0x8000;

// Control numbers, the positions in g_adwControlMasks.
enum Control {
    kControlUp = 0,
    kControlRight = 1,
    kControlDown = 2,
    kControlLeft = 3,
    kControlSelect = 4,
    kControlL3 = 5,
    kControlR3 = 6,
    kControlStart = 7,
    kControlL2 = 8,
    kControlR1 = 9,
    kControlL1 = 10,
    kControlR2 = 11,
    kControlTriangle = 12,
    kControlCircle = 13,
    kControlCross = 14,
    kControlSquare = 15,
};

// The four face buttons, the controls from kControlTriangle onward.
constexpr int kFaceButtonCount = 4;

constexpr int kControlTotal = kControlSquare + 1;

// 0x008efb60
// The button bit of each control, which Init() fills.
unsigned int g_adwControlMasks[kControlTotal];

// 0x00692440
// The control each stick axis reading reports.
int g_anAxisControls[] = {0x64, 0x65, 0x66, 0x67};

// The reading type every controller reading carries.
constexpr int kReadingTypeJoy = 0x6a6f7920; // 'joy '

// The value a press reports, and the value a release reports.
constexpr float kPressedValue = 0.99f;
constexpr float kReleasedValue = 0.0f;

// A stick axis byte, taken as signed, is offset by this much and divided by the range below.
constexpr double kAxisOffset = 128.0;
constexpr double kAxisRange = 255.01;

// Joypad::Read() results.
constexpr int kReadNoPad = 0;
constexpr int kReadBusy = 1;
constexpr int kReadReady = 2;

// The GrooveWorld::mState value in which the world accepts readings.
constexpr int kWorldStateAcceptingReadings = 4;

// The script template debug keys run, with the control number from 1.
constexpr int kDebugKeyTemplate = 0x3f2;

// The ports and the multitap slots Setup() opens.
constexpr int kPort0 = 0;
constexpr int kPort1 = 1;
constexpr int kSlotsPerPort = 4;

// A Joypad's slot index is its port shifted past the multitap slot bits.
constexpr int kSlotBits = 2;

// sceMtapGetConnection() reports 1 when a multitap is present.
constexpr int kMultitapConnected = 1;

// The value Setup() passes as Joypad::Open()'s third argument.
constexpr int kJoypadOpenUnknown128 = 4;

// Players are numbered from 1, and 0 marks a Joypad with no player.
constexpr int kNoPlayer = 0;
constexpr int kFirstPlayer = 1;
constexpr int kSecondPlayer = 2;

// The Joypad on slot 0 of port 1, which Setup() opens after the four of port 0.
constexpr int kPort1Joypad = kSlotsPerPort;

/**
 * Scheduler command the controller scan runs.
 *
 * `Q235_GLOBAL_$N$InputPollerPS2.cppXFKhgb24FindJoypadConnectionsCmd` at `0x007e6ae8` is its RTTI
 * name. Only the factory is recovered; no vtable that references the class has been located.
 */
class FindJoypadConnectionsCmd : public Sch::Command {
public:
    // 0x001e1990
    // The factory the unit's static initialiser at 0x001e1750 registers under identifier zero.
    static Sch::Command *NewCmd() {
        return nullptr;
    }
};

} // namespace

// 0x001df080
InputPoller::~InputPoller() {
    Shutdown();
}

// 0x001e19b8
void InputPoller::Init() {
    g_adwControlMasks[kControlUp] = kPadButtonUp;
    g_adwControlMasks[kControlRight] = kPadButtonRight;
    g_adwControlMasks[kControlDown] = kPadButtonDown;
    g_adwControlMasks[kControlLeft] = kPadButtonLeft;
    g_adwControlMasks[kControlSelect] = kPadButtonSelect;
    g_adwControlMasks[kControlL3] = kPadButtonL3;
    g_adwControlMasks[kControlR3] = kPadButtonR3;
    g_adwControlMasks[kControlStart] = kPadButtonStart;
    g_adwControlMasks[kControlL2] = kPadButtonL2;
    g_adwControlMasks[kControlR1] = kPadButtonR1;
    g_adwControlMasks[kControlL1] = kPadButtonL1;
    g_adwControlMasks[kControlR2] = kPadButtonR2;
    g_adwControlMasks[kControlTriangle] = kPadButtonTriangle;
    g_adwControlMasks[kControlCircle] = kPadButtonCircle;
    g_adwControlMasks[kControlCross] = kPadButtonCross;
    g_adwControlMasks[kControlSquare] = kPadButtonSquare;
    Setup();
}

// 0x001df248
void InputPoller::Setup() {
    for (int nSlot = 0; nSlot < kSlotsPerPort; ++nSlot) {
        Joypad *pJoypad = new Joypad((kPort0 << kSlotBits) | nSlot);
        pJoypad->Open(kPort0, nSlot, kJoypadOpenUnknown128);
        pJoypad->mId = mNextJoypadId;
        ++mNextJoypadId;
        mJoypads.push_back(pJoypad);
        Entry entry;
        for (int i = kControlCount - 1; i >= 0; --i) {
            entry.mUnknown00[i] = 0;
        }
        for (int i = kAxisCount - 1; i >= 0; --i) {
            entry.mAxes[i] = 0;
        }
        entry.mFaceButtonHeld = 0;
        entry.mButtons = 0;
        mEntries.push_back(entry);
    }

    Joypad *pJoypad = new Joypad(kPort1Joypad);
    pJoypad->Open(kPort1, 0, kJoypadOpenUnknown128);
    pJoypad->mId = mNextJoypadId;
    ++mNextJoypadId;
    mJoypads.push_back(pJoypad);
    Entry entry;
    for (int i = kControlCount - 1; i >= 0; --i) {
        entry.mUnknown00[i] = 0;
    }
    for (int i = kAxisCount - 1; i >= 0; --i) {
        entry.mAxes[i] = 0;
    }
    entry.mFaceButtonHeld = 0;
    entry.mButtons = 0;
    mEntries.push_back(entry);

    const int nJoypadCount = mJoypads.size();
    mJoypadPlayers.resize(nJoypadCount, kNoPlayer);
    for (int i = 0; i < nJoypadCount; ++i) {
        mJoypadPlayers[i] = kNoPlayer;
    }

    mMultitap0 = sceMtapGetConnection(kPort0) == kMultitapConnected;
    mMultitap1 = sceMtapGetConnection(kPort1) == kMultitapConnected;
    if (mMultitap0) {
        for (unsigned i = 0; i < mJoypadPlayers.size() - 1; ++i) {
            mJoypadPlayers[i] = i + kFirstPlayer;
        }
    } else {
        mJoypadPlayers[0] = kFirstPlayer;
        if (!mMultitap1) {
            mJoypadPlayers[kPort1Joypad] = kSecondPlayer;
        }
    }
}

// 0x001df9d8
void InputPoller::Shutdown() {
    if (mJoypads.size() == 0) {
        return;
    }
    for (auto it = mJoypads.begin(); it != mJoypads.end(); ++it) {
        (*it)->Close();
        delete *it;
    }
    mJoypads.clear();
    mEntries.clear();
}

// 0x001df798
void InputPoller::FindJoypadConnections() {
    if (!mActive) {
        return;
    }
    const int nMultitap0 = sceMtapGetConnection(kPort0);
    const int nMultitap1 = sceMtapGetConnection(kPort1);
    // The stored flag is compared against the raw connection value, not against a flag.
    if (mMultitap1 != nMultitap1) {
        for (auto it = mJoypads.begin(); it != mJoypads.end(); ++it) {
            (*it)->Reset();
        }
    }
    mMultitap1 = nMultitap1 != 0;

    if (nMultitap0 == kMultitapConnected) {
        if (mMultitap0) {
            return;
        }
        mMultitap0 = 1;
        for (unsigned i = 0; i < mJoypadPlayers.size() - 1; ++i) {
            mJoypadPlayers[i] = i + kFirstPlayer;
        }
        mJoypadPlayers[mJoypadPlayers.size() - 1] = kNoPlayer;
        for (auto it = mJoypads.begin(); it != mJoypads.end(); ++it) {
            (*it)->Reset();
        }
        return;
    }

    mJoypadPlayers[kPort1Joypad] = nMultitap1 == kMultitapConnected ? kNoPlayer : kSecondPlayer;
    if (mMultitap0) {
        for (auto it = mJoypads.begin(); it != mJoypads.end(); ++it) {
            (*it)->Reset();
        }
        mMultitap0 = 0;
        mJoypadPlayers[0] = kFirstPlayer;
        for (unsigned i = 1; i < mJoypadPlayers.size() - 1; ++i) {
            mJoypadPlayers[i] = kNoPlayer;
        }
    }
}

// 0x001dfab0
void InputPoller::ReadControllers() {
    mPressedThisPoll = 0;
    if (mController == nullptr) {
        return;
    }
    int nIndex = 0;
    FindJoypadConnections();
    GameManagerImpl *pGameManager = Application::shared()->GetGameManager();
    GrooveWorld *pWorld = Application::shared()->GetWorld();
    mUnknown50 = 0;

    for (auto it = mJoypads.begin(); it != mJoypads.end(); ++it, ++nIndex) {
        unsigned int dwButtons;
        unsigned char axes[kAxisCount];
        const int nResult = (*it)->Read(&dwButtons, &axes[0], &axes[1], &axes[2], &axes[3]);
        const int nId = (*it)->mId;

        if (nResult == kReadNoPad) {
            if (mGameInputEnabled && pWorld != nullptr &&
                pWorld->mState == kWorldStateAcceptingReadings && !mPaused) {
                const int nPlayer = mJoypadPlayers[nId];
                if (nPlayer != kNoPlayer &&
                    !(static_cast<int>(pWorld->mLocalPlayers.size()) < nPlayer)) {
                    PauseGameSystemMsg pause;
                    pGameManager->QueueMessage(&pause);
                    pWorld->mInputMap->StopAllRiffs();
                    mPaused = 1;
                }
            }
            continue;
        }
        if (nResult == kReadBusy) {
            mUnknown50 = nResult;
            continue;
        }
        if (nResult < kReadReady) {
            continue;
        }

        const int nPlayer = mJoypadPlayers[nId];
        if (nPlayer == kNoPlayer) {
            return; // Yes, the binary stops reading the remaining Joypads here.
        }

        Entry &entry = mEntries[nIndex];
        const unsigned int dwPrevious = entry.mButtons;
        entry.mButtons = dwButtons;
        const unsigned int dwChanged = dwButtons ^ dwPrevious;
        const unsigned int dwPressed = dwChanged & dwButtons;
        const unsigned int dwReleased = dwChanged & ~dwButtons;
        if (!(dwButtons & g_adwControlMasks[kControlTriangle]) &&
            !(dwButtons & g_adwControlMasks[kControlCircle]) &&
            !(dwButtons & g_adwControlMasks[kControlCross]) &&
            !(dwButtons & g_adwControlMasks[kControlSquare])) {
            entry.mFaceButtonHeld = 0;
        }

        for (int nControl = 0; nControl < kControlTotal;) {
            if (!(dwPressed & g_adwControlMasks[nControl])) {
                ++nControl;
                if (dwReleased & g_adwControlMasks[nControl - 1]) {
                    mController->OnUnknownSlot2(kReadingTypeJoy, nPlayer, nControl, kReleasedValue);
                }
                continue;
            }
            if (static_cast<unsigned>(nControl - kControlTriangle) <
                static_cast<unsigned>(kFaceButtonCount)) {
                if (entry.mFaceButtonHeld) {
                    break; // Yes, a second face button ends the scan of the remaining controls.
                }
                entry.mFaceButtonHeld = 1;
            }
            if (DebugKeysEnabled() && nControl != kControlSelect &&
                (dwButtons & kPadButtonSelect)) {
                ++nControl;
                if (dwButtons & kPadButtonR3) {
                    CallScriptTemplate(kDebugKeyTemplate, nControl);
                }
            } else {
                mController->OnUnknownSlot2(kReadingTypeJoy, nPlayer, nControl + 1, kPressedValue);
                ++nControl;
            }
            mPressedThisPoll = 1;
        }

        for (int nAxis = 0; nAxis < kAxisCount; ++nAxis) {
            // The byte is taken as signed.
            const char nPosition = static_cast<char>(axes[nAxis]);
            if (nPosition == entry.mAxes[nAxis]) {
                continue;
            }
            entry.mAxes[nAxis] = nPosition;
            const float flValue = static_cast<float>((nPosition + kAxisOffset) / kAxisRange);
            mController->OnUnknownSlot2(kReadingTypeJoy, nPlayer, g_anAxisControls[nAxis], flValue);
        }
    }
}

// 0x001e1c28
void InputPoller::Poll() {
    ReadControllers();
    OnUnknown001e1c58();
}

// 0x001e1c58
void InputPoller::OnUnknown001e1c58() {
}

// 0x001e1998
void InputPoller::SetController(RawController *pController) {
    mController = pController;
}

// 0x001e1a80
void InputPoller::SetActive(int bActive) {
    mActive = bActive;
}

// 0x001e19a0
void InputPoller::DetachController(RawController *pController) {
    if (mController == pController) {
        mController = nullptr;
    }
}

// 0x001e1c18
void InputPoller::SetPaused(int bPaused) {
    mPaused = bPaused;
}
