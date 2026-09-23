#include "game/inputmap.h"

#include "app/application.h"
#include "app/globals.h"
#include "app/watchdogtimer.h"
#include "game/gamemanagerimpl.h"
#include "game/player.h"
#include "msg/advancesectionmsg.h"
#include "msg/axisfxmsg.h"
#include "msg/axisregistermsg.h"
#include "msg/axisxpowmsg.h"
#include "msg/axisypowmsg.h"
#include "msg/buttonpowmsg.h"
#include "msg/erasemsg.h"
#include "msg/looptoolmsg.h"
#include "msg/message.h"
#include "msg/pitchriffmsg.h"
#include "msg/playbackmodemsg.h"
#include "msg/rawcontrollermsg.h"
#include "msg/rotleftmsg.h"
#include "msg/rotrightmsg.h"
#include "msg/stopriffmsg.h"
#include "msg/toggleghostmsg.h"
#include "sch/tickclock.h"

namespace {

// Bit positions MakeKey() packs the port and the device above.
constexpr int kPortShift = 5;
constexpr int kButtonShift = 16;

// The enable word a new binding starts with.
constexpr int kBindingEnabled = 1;

// Axis quantisation thresholds, compared in double precision. A value between two bands leaves
// the step unchanged.
constexpr double kAxisLowBelow = 0.2;
constexpr double kAxisCentreAbove = 0.25;
constexpr double kAxisCentreBelow = 0.75;
constexpr double kAxisHighAbove = 0.8;

constexpr int kAxisStepLow = -1;
constexpr int kAxisStepCentre = 0;
constexpr int kAxisStepHigh = 1;

// Largest gap between two erase presses that marks the second as a double tap.
constexpr long long kDoubleTapNanoseconds = 400000000;

// ButtonPowMsg's configured argument for a `powb` press.
constexpr int kButtonPowPressed = 1;

} // namespace

InputMap *g_pInputMap;

// 0x00119160
InputMap::InputMap(Globals *pGlobals, std::vector<Player *> *pPlayers) : mGlobals(pGlobals) {
    mPlayers = pPlayers;
    g_pInputMap = this;
    for (unsigned i = 0; i < kSlotCount; ++i) {
        for (unsigned j = 0; j < kRiffCount; ++j) {
            mUnknown30[i][j] = 0;
        }
    }
}

// 0x001193d0
InputMap::~InputMap() {
    g_pInputMap = nullptr;
}

// 0x0011dc20
void InputMap::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nRawControllerMsgType) {
        OnControllerReading(static_cast<RawControllerMsg *>(pMsg));
    }
}

// 0x00119518
void InputMap::OnControllerReading(RawControllerMsg *pMsg) {
    const MetControllerReading &reading = pMsg->mReading;
    const int nKey = MakeKey(reading.mTag, reading.mPadIndex, reading.mButton);
    const auto found = mBindingMap.find(nKey);
    if (found == mBindingMap.end()) {
        return;
    }

    Binding &binding = *found->second;
    if (binding.mEnabled == 0) {
        return;
    }

    float flValue = reading.mValue;
    if (binding.mState != nullptr) {
        int nStep;
        if (flValue < kAxisLowBelow) {
            nStep = kAxisStepLow;
        } else if (flValue > kAxisCentreAbove && flValue < kAxisCentreBelow) {
            nStep = kAxisStepCentre;
        } else if (flValue > kAxisHighAbove) {
            nStep = kAxisStepHigh;
        } else {
            return;
        }

        if (nStep == *binding.mState) {
            return;
        }
        *binding.mState = nStep;
        flValue = static_cast<float>(nStep);
    }

    Player *pPlayer = nullptr;
    for (auto it = mPlayers->begin(); it != mPlayers->end(); ++it) {
        if ((*it)->Slot2() == binding.mSlot) {
            pPlayer = *it;
            break;
        }
    }
    if (pPlayer == nullptr) {
        return;
    }

    switch (binding.mAction) {
    case kActionRotateLeft:
        if (flValue != 0.0) {
            RotLeftMsg msg(pPlayer, pMsg->mPosition);
            Send(&msg);
        }
        break;
    case kActionRotateRight:
        if (flValue != 0.0) {
            RotRightMsg msg(pPlayer, pMsg->mPosition);
            Send(&msg);
        }
        break;
    case kActionPlayback:
        if (flValue != 0.0) {
            PlaybackModeMsg msg(pPlayer, pMsg->mPosition);
            Send(&msg);
        }
        break;
    case kActionPitchRiff:
        if (flValue != 0.0) {
            SendPitchRiff(pMsg->mPosition, pPlayer, pPlayer->Slot4(), binding.mExtra);
        } else {
            SendStopRiff(pMsg->mPosition, pPlayer, pPlayer->Slot4(), binding.mExtra);
        }
        break;
    case kActionAxisRegister: {
        AxisRegisterMsg msg(pPlayer, flValue, pMsg->mPosition, pPlayer->Slot4());
        Send(&msg);
        break;
    }
    case kActionAxisFX: {
        AxisFXMsg msg(pPlayer, flValue, pMsg->mPosition, pPlayer->Slot4());
        Send(&msg);
        break;
    }
    case kActionErase:
        if (flValue != 0.0) {
            const int nPlayMode = mGlobals->GetPlayMode();
            if (nPlayMode == kPlayModeGame) {
                ButtonPowMsg msg(pPlayer, nPlayMode, pMsg->mPosition);
                Send(&msg);
            } else {
                const long long nNow = Application::shared()->GetWatchdogTimer()->Now();
                const int bDoubleTap = (nNow - pPlayer->mLastEraseTime) < kDoubleTapNanoseconds;
                EraseMsg msg(pPlayer, pMsg->mPosition, pPlayer->Slot4(), bDoubleTap);
                Send(&msg);
                pPlayer->mLastEraseTime = nNow;
            }
        }
        break;
    case kActionAdvance:
        if (flValue != 0.0) {
            AdvanceSectionMsg msg(pPlayer, pMsg->mPosition, pPlayer->Slot4());
            Send(&msg);
        }
        break;
    case kActionLoop:
        if (flValue != 0.0) {
            LoopToolMsg msg(pPlayer, pMsg->mPosition, pPlayer->Slot4());
            Send(&msg);
        }
        break;
    case kActionAxisX: {
        AxisXPowMsg msg(pPlayer, static_cast<int>(flValue), pMsg->mPosition);
        Send(&msg);
        break;
    }
    case kActionAxisY: {
        AxisYPowMsg msg(pPlayer, static_cast<int>(flValue), pMsg->mPosition);
        Send(&msg);
        break;
    }
    case kActionButtonPow:
        if (flValue != 0.0) {
            ButtonPowMsg msg(pPlayer, kButtonPowPressed, pMsg->mPosition);
            Send(&msg);
        }
        break;
    case kActionGhost:
        if (flValue != 0.0) {
            ToggleGhostMsg msg;
            Send(&msg);
        }
        break;
    default:
        break;
    }
}

// 0x0011da68
void InputMap::SendStopRiff(Mid::MBT position, Player *pPlayer, int nTrack, int nRiff) {
    StopRiffMsg msg(nRiff, pPlayer, position, nTrack);
    Send(&msg);
}

// 0x0011d9b0
void InputMap::SendPitchRiff(Mid::MBT position, Player *pPlayer, int nTrack, int nRiff) {
    pPlayer->Slot2(); // Yes, the binary discards this call's result.
    PitchRiffMsg msg(nRiff, pPlayer, position, nTrack);
    Send(&msg);
}

InputMap *InputMap::shared() {
    return g_pInputMap;
}

// 0x0011dc08
int InputMap::MakeKey(int nDevice, int nPort, int nButton) {
    return ((nDevice << kPortShift | nPort) << kButtonShift) | nButton;
}

// 0x0011db78
void InputMap::DisableEntries() {
    for (std::list<Binding>::iterator it = mBindings.begin(); it != mBindings.end(); ++it) {
        it->mEnabled = 0;
    }
}

// 0x0011dbc0
void InputMap::EnableEntries() {
    for (std::list<Binding>::iterator it = mBindings.begin(); it != mBindings.end(); ++it) {
        it->mEnabled = 1;
    }
}

// 0x0011db18
void InputMap::SetEnabled(int nSlot, int nAction, int nEnabled) {
    for (std::list<Binding>::iterator it = mBindings.begin(); it != mBindings.end(); ++it) {
        if (it->mSlot == nSlot && it->mAction == nAction) {
            it->mEnabled = nEnabled;
        }
    }
}

// 0x00119f58
std::list<InputMap::Binding>::iterator InputMap::FindOrAddBinding(const Binding &binding) {
    for (std::list<Binding>::iterator it = mBindings.begin(); it != mBindings.end(); ++it) {
        if (it->mSlot == binding.mSlot && it->mAction == binding.mAction &&
            it->mExtra == binding.mExtra) {
            return it;
        }
    }

    int *pState = nullptr;
    if (binding.mAction == kActionAxisX || binding.mAction == kActionAxisY) {
        pState = new int(0);
    }
    std::list<Binding>::iterator it = mBindings.insert(mBindings.end(), binding);
    it->mState = pState;
    return it;
}

// 0x0011a0f0
void InputMap::AddBinding(int nDevice, int nPort, int nButton, int nSlot, int nAction, int nExtra) {
    const int nKey = MakeKey(nDevice, nPort, nButton);
    const Binding binding = {nSlot, nAction, nExtra, kBindingEnabled, nullptr};
    mBindingMap[nKey] = FindOrAddBinding(binding);
}

// 0x00119dd0
void InputMap::StopAllRiffs() {
    const int nNow = mGlobals->GetSongClock()->SongTick();
    for (unsigned i = 0; i < mPlayers->size(); ++i) {
        for (int nRiff = 0; nRiff < kRiffCount; ++nRiff) {
            Player *pPlayer = (*mPlayers)[i];
            StopRiffMsg msg;
            msg.mUnknown10 = pPlayer->Slot4();
            msg.mUnknown04 = nRiff;
            msg.mPlayer = pPlayer;
            msg.mPosition.mTick = nNow;
            Send(&msg);
        }
    }

    for (unsigned i = 0; i < kSlotCount; ++i) {
        for (unsigned j = 0; j < kRiffCount; ++j) {
            mUnknown30[i][j] = 0;
        }
    }
}

// 0x0011d1a0
void InputMap::ClearBindingMap() {
    mBindingMap.clear();
}
