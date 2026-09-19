#include "game/gamemanagerimpl.h"

#include <vector>

#include "met/metpersonadata.h"
#include "msg/begingamelocalmsg.h"
#include "msg/endgamemsg.h"
#include "msg/gamemanagerdoplaybackmsg.h"
#include "msg/pausegamesystemmsg.h"
#include "msg/unpausegamesystemmsg.h"
#include "os/log.h"
#include "script/scripthost.h"

namespace {

// Script templates the manager publishes its settings through.
constexpr int kScriptTemplateGameMode = 0x262;
constexpr int kScriptTemplatePlayMode = 0x263;
constexpr int kScriptTemplateUnknown88 = 0x264;

} // namespace

int GameManagerImpl::CheckState() {
    // Yes, the binary branches on the state and then returns 1 either way. The instruction that
    // looks like the taken path is the branch-likely delay slot.
    if (mState != 0) {
        return 1;
    }
    return 1;
}

int GameManagerImpl::GetUnknownfc() {
    return mUnknownfc;
}

void GameManagerImpl::AddPersona(const MetPersonaData &persona) {
    MetPersonaData *pPersona = new MetPersonaData;
    *pPersona = persona;
    mPersonas.push_back(pPersona);
}

std::vector<MetPersonaData *> *GameManagerImpl::GetPersonas() {
    return &mPersonas;
}

void GameManagerImpl::ClearPersonas() {
    for (std::vector<MetPersonaData *>::iterator it = mPersonas.begin(); it != mPersonas.end();
         ++it) {
        delete *it;
        *it = nullptr;
    }
    mPersonas.erase(mPersonas.begin(), mPersonas.end());
}

GrooveWorld *GameManagerImpl::GetWorld() {
    return mpWorld;
}

MetaGameWorld *GameManagerImpl::GetMetaWorld() {
    return mpMetaWorld;
}

InputPoller *GameManagerImpl::GetPoller() {
    return mpPoller;
}

int GameManagerImpl::GetUnknown18() {
    return mUnknown18;
}

GameStats *GameManagerImpl::GetStats() {
    return &mStats;
}

void GameManagerImpl::Save(OBStream *pStream) {
    pStream->Write(&mState, sizeof(mState))
        .Write(&mUnknown08, sizeof(mUnknown08))
        .Write(&mGameMode, sizeof(mGameMode));
    mParams.Save(pStream);
}

int GameManagerImpl::IsPlaybackActive() {
    return mpPlayback != nullptr;
}

void GameManagerImpl::SetGameMode(int nMode) {
    const char *pszName = "";
    mGameMode = nMode;
    switch (nMode) {
    case kGameModeNone:
        pszName = "none";
        break;
    case kGameModeSolo:
        pszName = "solo";
        break;
    case kGameModeLocal:
        pszName = "local";
        break;
    case kGameModeNet:
        pszName = "net";
        break;
    }
    CallScriptTemplate(kScriptTemplateGameMode, pszName);
    mParams.mUnknown28 = nMode == kGameModeNet;
    ++mChangeCount;
}

int GameManagerImpl::GetGameMode() {
    return mGameMode;
}

GameParams *GameManagerImpl::GetParams() {
    return &mParams;
}

int GameManagerImpl::GetChangeCount() {
    return mChangeCount;
}

void GameManagerImpl::SetParams(const GameParams &params) {
    CheckState(); // Yes, the binary discards this call's result.
    mParams = params;
    // The two modes are republished from the settings just copied in, not from the argument.
    SetPlayMode(mParams.mUnknown1c);
    SetUnknown88(mParams.mUnknown20);
    ++mChangeCount;
    CheckState(); // Yes, the binary discards this call's result.
}

void GameManagerImpl::SetUnknown88(int nValue) {
    mParams.mUnknown20 = nValue;
    // No literal maps the value, and the raw word goes out as the template argument.
    CallScriptTemplate(kScriptTemplateUnknown88, nValue);
    ++mChangeCount;
}

void GameManagerImpl::SetPlayMode(int nMode) {
    const char *pszName = "";
    mParams.mUnknown1c = nMode;
    switch (nMode) {
    case kPlayModeNone:
        pszName = "none";
        break;
    case kPlayModeGame:
        pszName = "game";
        break;
    case kPlayModeJam:
        pszName = "jam";
        break;
    }
    CallScriptTemplate(kScriptTemplatePlayMode, pszName);
    ++mChangeCount;
}

int GameManagerImpl::GetUnknown88() {
    return mParams.mUnknown20;
}

int GameManagerImpl::GetPlayMode() {
    return mParams.mUnknown1c;
}

void GameManagerImpl::SetDrawEnabled(int nEnabled) {
    // Yes, the binary inverts the low bit rather than the whole value, so 2 records 3.
    mDrawSuppressed = nEnabled ^ 1;
}

void GameManagerImpl::HandleMessage(Message *pMsg) {
    int nType = pMsg->Type();
    if (nType == g_nBeginGameLocalMsgType) {
        OnBeginGameLocal(pMsg);
    } else if (nType == g_nEndGameMsgType) {
        OnEndGame(pMsg);
    } else if (nType == g_nPauseGameSystemMsgType) {
        OnPauseGameSystem(pMsg);
    } else if (nType == g_nUnpauseGameSystemMsgType) {
        OnUnpauseGameSystem(pMsg);
    } else if (nType == g_nGameManagerDoPlaybackMsgType) {
        OnDoPlayback(pMsg);
    } else {
        // The format string has no placeholder, so the name is formatted into nothing.
        Fatal("DISPATCH_CHECK: ", pMsg->Name());
    }
}
