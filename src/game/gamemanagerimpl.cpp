#include "game/gamemanagerimpl.h"

#include <vector>

#include "app/application.h"
#include "app/rendererbase.h"
#include "app/watchdog.h"
#include "game/forcefeedbackmgr.h"
#include "game/gameplayback.h"
#include "game/gamerecorder.h"
#include "game/inputmap.h"
#include "met/metpersonadata.h"
#include "msg/begingamelocalmsg.h"
#include "msg/endgamemsg.h"
#include "msg/gamemanagerdoplaybackmsg.h"
#include "msg/metstartpausemsg.h"
#include "msg/pausegamesystemmsg.h"
#include "msg/unpausegamesystemmsg.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "script/configquery.h"
#include "script/scripthost.h"
#include "synth/ps2hardsynth.h"

namespace {

// Script templates the manager publishes its settings through.
constexpr int kScriptTemplateGameMode = 0x262;
constexpr int kScriptTemplatePlayMode = 0x263;
constexpr int kScriptTemplateUnknown88 = 0x264;
constexpr int kScriptTemplateLevelName = 0x277;
constexpr int kScriptTemplateArenaName = 0x27b;

// Configuration code of the container name CreateWorld() hands the world.
constexpr int kContainerConfigCode = 0x38e;

// The diagnostics StartRecording() and StartPlayback() trip.
constexpr char kRecordingInProgress[] = "Recording already in progress";
constexpr char kCannotStartRecording[] = "Cannot start recording from this state";
constexpr char kCannotRecreateGame[] = "Cannot recreate game from this state";
constexpr char kPlaybackInProgress[] = "Playback already in progress";

// The MIDI message a pause sends: all notes off, controller 123, on the last channel.
constexpr unsigned char kStatusControlChangeChannel16 = 0xbf;
constexpr unsigned char kControllerAllNotesOff = 123;

// Configuration code of the recording OnDoPlayback() replays.
constexpr int kPlaybackFileConfigCode = 0x26a;

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

// 0x00105f50
GameManagerImpl::GameManagerImpl()
    : mState(0), mUnknown08(0), mpWorld(nullptr), mpMetaWorld(nullptr), mUnknown18(0), mGameMode(0),
      mChangeCount(0), mUnknowna8(0), mpRecorder(nullptr), mpPlayback(nullptr), mUnknownfc(1),
      mUnknown100(0), mPaused(0), mDrawSuppressed(1) {
    mQueue.AddSink(this);
    mpPoller = new InputPoller;
    mpPoller->mUnknown34 = 0;
    CheckState(); // Yes, the binary discards this call's result.
}

// 0x001062d0
GameManagerImpl::~GameManagerImpl() {
    CheckState(); // Yes, the binary discards this call's result.
    delete mpRecorder;
    mpRecorder = nullptr;
    delete mpMetaWorld;
    mpMetaWorld = nullptr;
    delete mpPoller;
    mpPoller = nullptr;
}

// 0x001068a0
void GameManagerImpl::CreateWorld() {
    mpWorld = new GrooveWorld(Application::shared(), &mStats);
    const HxStr &level = mParams.mLevelName;
    CallScriptTemplate(kScriptTemplateLevelName,
                       level.mStr != nullptr ? level.mStr : g_szEmptyString);
    const HxStr &arena = mParams.mArenaName;
    CallScriptTemplate(kScriptTemplateArenaName,
                       arena.mStr != nullptr ? arena.mStr : g_szEmptyString);

    HxStr container;
    QueryConfigString(&container, kContainerConfigCode);
    mpWorld->StartLoad(container);
}

// 0x0010c168
void GameManagerImpl::Start() {
    mpMetaWorld = new MetaGameWorld;
    mpPoller->SetController(mpMetaWorld);
    mUnknowna8 = 1;
    mpPoller->SetActive(1);
}

// 0x001069a8
void GameManagerImpl::OnPauseGameSystem(Message *) {
    if (mPaused != 0) {
        return;
    }

    mPaused = 1;
    if (GetGameMode() != kGameModeNet) {
        Application::shared()->GetWatchdog()->mClock.Pause();
    }
    mpPoller->SetController(mpMetaWorld);
    mpPoller->SetPaused(1);
    Application::shared()->GetSynth()->SendMidi(
        kStatusControlChangeChannel16, kControllerAllNotesOff, 0);
    Application::shared()->GetSynth()->Slot14(1);
    if (mpWorld != nullptr) {
        mpWorld->mForceFeedback->SetPaused(1);
    }

    MetStartPauseMsg pause;
    mpMetaWorld->GetRenderer()->Handle(&pause);
}

// 0x0010c148
void GameManagerImpl::OnEndGame(Message *pMsg) {
    EndGame(static_cast<EndGameMsg *>(pMsg)->mRestart);
}

// 0x00106af8
void GameManagerImpl::OnUnpauseGameSystem(Message *) {
    if (mPaused == 0) {
        return;
    }

    mPaused = 0;
    mpMetaWorld->OnUnknownForwarder003d4890();
    mpPoller->SetController(mpWorld);
    mpPoller->SetPaused(0);
    if (GetWorld()->mUnknown90 == 0) {
        GetWorld()->mInputMap->Rebuild();
    }
    Application::shared()->GetSynth()->Slot14(0);
    if (mpWorld != nullptr) {
        mpWorld->mForceFeedback->SetPaused(0);
    }
    if (GetGameMode() != kGameModeNet) {
        Application::shared()->GetWatchdog()->mClock.Resume();
    }
}

// 0x0010c0c0
void GameManagerImpl::FinishWorldLoad() {
    mUnknownfc = 1;
    while (mpWorld->IsLoadDone() == 0) {
    }
    mpWorld->FinishLoad();
    AddPlayers();
    mpWorld->PrepareLevel();
    mpPoller->SetController(mpWorld);
}

// 0x0010c1f0
void GameManagerImpl::AddPlayers() {
    AddPersonaPlayers();
}

// 0x0010c420
void GameManagerImpl::StartRecording() {
    if (mpRecorder != nullptr) {
        Fatal(kRecordingInProgress);
    }
    if (mState != 0) {
        Fatal(kCannotStartRecording);
    }
    mpRecorder = new GameRecorder(this);
}

// 0x0010c4b8
void GameManagerImpl::StartPlayback(const HxStr &file, int nFlag) {
    if (mState != 0) {
        Fatal(kCannotRecreateGame);
    }
    if (mpPlayback != nullptr) {
        Fatal(kPlaybackInProgress);
    }
    if (mpRecorder != nullptr) {
        delete mpRecorder;
    }
    mpRecorder = nullptr;
    mpMetaWorld->OnUnknownForwarder003d4890();
    mpPlayback = new GamePlayback(file, this, nFlag);
}

// 0x0010bee0
void GameManagerImpl::QueueMessage(Message *pMsg) {
    MsgSink *pQueueSink = &mQueue;
    pQueueSink->HandleMessage(pMsg);
}

// 0x0010bf10
void GameManagerImpl::OnDoPlayback(Message *) {
    HxStr file;
    QueryConfigString(&file, kPlaybackFileConfigCode);
    StartPlayback(file, 0);
}
