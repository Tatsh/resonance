#include "game/inputmap.h"

#include "app/globals.h"
#include "game/player.h"
#include "msg/message.h"
#include "msg/rawcontrollermsg.h"
#include "msg/stopriffmsg.h"
#include "sch/tickclock.h"

namespace {

// Bit positions MakeKey() packs the port and the device above.
constexpr int kPortShift = 5;
constexpr int kButtonShift = 16;

// The enable word a new binding starts with.
constexpr int kBindingEnabled = 1;

} // namespace

InputMap *g_pInputMap;

InputMap::InputMap(Globals *pGlobals, std::vector<Player *> *pPlayers) : mGlobals(pGlobals) {
    mPlayers = pPlayers;
    g_pInputMap = this;
    for (unsigned i = 0; i < kSlotCount; ++i) {
        for (unsigned j = 0; j < kRiffCount; ++j) {
            mUnknown30[i][j] = 0;
        }
    }
}

InputMap::~InputMap() {
    g_pInputMap = nullptr;
}

void InputMap::HandleMessage(Message *pMsg) {
    if (pMsg->Type() == g_nRawControllerMsgType) {
        OnControllerReading(pMsg);
    }
}

InputMap *InputMap::shared() {
    return g_pInputMap;
}

int InputMap::MakeKey(int nDevice, int nPort, int nButton) {
    return ((nDevice << kPortShift | nPort) << kButtonShift) | nButton;
}

void InputMap::DisableEntries() {
    for (std::list<Binding>::iterator it = mBindings.begin(); it != mBindings.end(); ++it) {
        it->mEnabled = 0;
    }
}

void InputMap::EnableEntries() {
    for (std::list<Binding>::iterator it = mBindings.begin(); it != mBindings.end(); ++it) {
        it->mEnabled = 1;
    }
}

void InputMap::SetEnabled(int nSlot, int nAction, int nEnabled) {
    for (std::list<Binding>::iterator it = mBindings.begin(); it != mBindings.end(); ++it) {
        if (it->mSlot == nSlot && it->mAction == nAction) {
            it->mEnabled = nEnabled;
        }
    }
}

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

void InputMap::AddBinding(int nDevice, int nPort, int nButton, int nSlot, int nAction, int nExtra) {
    const int nKey = MakeKey(nDevice, nPort, nButton);
    const Binding binding = {nSlot, nAction, nExtra, kBindingEnabled, nullptr};
    mBindingMap[nKey] = FindOrAddBinding(binding);
}

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

void InputMap::ClearBindingMap() {
    mBindingMap.clear();
}
