#include "game/gamemanagerimpl.h"

#include <utility>
#include <vector>

#include "app/application.h"
#include "app/renderer.h"
#include "app/rendererbase.h"
#include "app/watchdog.h"
#include "app/watchdogtimer.h"
#include "game/dogamesystemplaycmd.h"
#include "game/forcefeedbackmgr.h"
#include "game/gameplayback.h"
#include "game/gamerecorder.h"
#include "game/inputmap.h"
#include "gfx/gfxdevice.h"
#include "memcard/memcardmanager.h"
#include "met/metpersonadata.h"
#include "msg/begingamelocalmsg.h"
#include "msg/endgamemsg.h"
#include "msg/gamemanagerdoplaybackmsg.h"
#include "msg/isrecordingmsg.h"
#include "msg/metfreqendedmsg.h"
#include "msg/metstartpausemsg.h"
#include "msg/pausegamesystemmsg.h"
#include "msg/unpausegamesystemmsg.h"
#include "os/cycles.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/r250.h"
#include "sch/cmdid.h"
#include "sch/tick.h"
#include "script/configquery.h"
#include "script/scripthost.h"
#include "synth/ps2hardsynth.h"

namespace {

// Script templates the manager publishes its settings through.
constexpr int kScriptTemplateGameMode = 0x262;
constexpr int kScriptTemplatePlayMode = 0x263;
constexpr int kScriptTemplateDifficulty = 0x264;
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

// DrawFrame() draws the game world's renderer and the front end's, at most one of each.
constexpr int kMaxDrawRoots = 2;

// PresentFrame() argument that presents without flipping the framebuffer.
constexpr int kNoBufferSwap = 0;

// The handle a post starts with, before the scheduler allocates one.
constexpr int kUnallocatedCommand = -2;

// The command that starts play is itself recorded.
constexpr int kRecordable = 1;

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
    SetDifficulty(mParams.mDifficulty);
    ++mChangeCount;
    CheckState(); // Yes, the binary discards this call's result.
}

void GameManagerImpl::SetDifficulty(int nDifficulty) {
    mParams.mDifficulty = nDifficulty;
    // No literal maps the value, and the raw word goes out as the template argument.
    CallScriptTemplate(kScriptTemplateDifficulty, nDifficulty);
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

int GameManagerImpl::GetDifficulty() {
    return mParams.mDifficulty;
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
    mpPoller->ClearUnknown34();
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

// 0x00106c08
void GameManagerImpl::EndGame(int bRestart) {
    CheckState(); // Yes, the binary discards this call's result.
    const int nUnknownb8 = mpWorld->mUnknownb8;
    Application::shared()->GetWatchdog()->Snapshot();
    mpPoller->DetachController(mpWorld);
    delete mpWorld;
    mpWorld = nullptr;
    if (mpRecorder != nullptr) {
        mpRecorder->ScheduleEnd();
    }
    if (mpPlayback != nullptr) {
        delete mpPlayback;
        mpPlayback = nullptr;
        SetGameMode(kGameModeNone);
        Application::shared()->GetWatchdog()->Snapshot();
    }

    if (bRestart != 0) {
        mUnknown100 = 1;
        BeginGameLocalMsg begin;
        QueueMessage(&begin);
    } else {
        mpPoller->SetController(mpMetaWorld);
        mpPoller->SetActive(1);
        mUnknowna8 = 1;
        MetFreqEndedMsg ended;
        ended.mUnknownb8Clear = nUnknownb8 == 0;
        mpMetaWorld->GetRenderer()->Handle(&ended);
    }
    CheckState(); // Yes, the binary discards this call's result.
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

// 0x001065a8
void GameManagerImpl::DrawFrame() {
    mQueue.Poll();

    RendererBase *roots[kMaxDrawRoots];
    int nRootCount = 0;
    if (mpWorld != nullptr && mpWorld->GetRendererSink() != nullptr) {
        roots[nRootCount++] = mpWorld->GetRendererSink();
    }
    if (mpMetaWorld != nullptr) {
        roots[nRootCount++] = mpMetaWorld->GetRenderer();
    }
    for (int i = 0; i < nRootCount; ++i) {
        roots[i]->OnUnknownSlot6();
        roots[i]->OnUnknownSlot7();
    }
    if (mUnknowna8 != 0) {
        MemcardManager::shared()->Update();
    }
    if (mDrawSuppressed != 0) {
        return;
    }

    g_gfxDevice.BeginFrame();
    g_gfxDevice.EnterVu1Path();
    for (int i = 0; i < nRootCount; ++i) {
        roots[i]->OnUnknownSlot8();
    }
    g_gfxDevice.LeaveVu1Path();
    g_gfxDevice.PresentFrame(kNoBufferSwap);
}

// 0x0010bfa0
void GameManagerImpl::DrawFrameSimple() {
    if (mpMetaWorld == nullptr || mpWorld != nullptr || mDrawSuppressed != 0) {
        return;
    }
    mpMetaWorld->GetRenderer()->OnUnknownSlot9();
    g_gfxDevice.BeginFrame();
    g_gfxDevice.EnterVu1Path();
    mpMetaWorld->GetRenderer()->OnUnknownSlot10();
    g_gfxDevice.LeaveVu1Path();
    g_gfxDevice.PresentFrame(kNoBufferSwap);
}

// 0x00106e28
void GameManagerImpl::PollPlayback() {
    mpPoller->Poll();
    Application::shared()->GetWatchdog(); // Yes, the binary discards this call's result.
    GetElapsedMilliseconds();             // Yes, the binary discards the reading.
    if (mpPoller->GetPressedThisPoll() != 0 && mpPlayback != nullptr && mpWorld != nullptr) {
        mpWorld->PostExitMode1();
    }
}

// 0x00106720
void GameManagerImpl::OnBeginGameLocal(Message *) {
    CheckState(); // Yes, the binary discards this call's result.
    if (mUnknown100 != 0) {
        mUnknown100 = 0;
    } else {
        mpMetaWorld->OnUnknownForwarder003d4890();
    }
    mpPoller->SetActive(0);
    mUnknowna8 = 0;
    {
        IsRecordingMsg recording;
        recording.mIsRecording = 0;
        mpMetaWorld->GetRenderer()->Handle(&recording);
    }

    CreateWorld();
    FinishWorldLoad();
    mpWorld->mUnknown8c = 0;
    mpPoller->SetGameInputEnabled(!Application::shared()->IsJukeboxMode());
    if (mpRecorder != nullptr) {
        mpRecorder->BeginRecording(mGameMode, mParams);
    }
    Application::shared()->GetWatchdog()->Flush();

    DoGameSystemPlayCmd *pCommand = new DoGameSystemPlayCmd;
    CmdID id;
    id.mValue = kUnallocatedCommand;
    const Sch::Tick now{0};
    Application::shared()->GetWatchdogTimer()->PostIn(pCommand, now, id, kRecordable);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
    CheckState(); // Yes, the binary discards this call's result.
}

// 0x001072b0
void GameManagerImpl::Load(IBStream *pStream) {
    int nState;
    pStream->Read(&nState, sizeof(nState));
    int nUnknown08;
    pStream->Read(&nUnknown08, sizeof(nUnknown08));
    int nGameMode;
    pStream->Read(&nGameMode, sizeof(nGameMode));
    mParams.Load(pStream);
    mState = nState;
    mUnknown08 = nUnknown08;
    mGameMode = nGameMode;
    SetGameMode(nGameMode);
    SetPlayMode(mParams.mUnknown1c);
    SetDifficulty(mParams.mDifficulty);

    ClearPersonas();
    MetPersonaData persona;
    persona.mUnknown140.mUnknown00 = HxStr("freq player 1");
    AddPersona(persona);
    {
        IsRecordingMsg recording;
        recording.mIsRecording = 1;
        mpMetaWorld->GetRenderer()->Handle(&recording);
    }

    Renderer::LoadLevel(mParams);
    CreateWorld();
    FinishWorldLoad(); // The binary expands this body inline here.
    mpWorld->mUnknown8c = 1;
    mpPoller->SetGameInputEnabled(0);
}

// 0x0010c128
void GameManagerImpl::OnUnknownSlot6() {
    mpWorld->StartPlay();
}

// 0x0010c1f0
void GameManagerImpl::AddPlayers() {
    AddPersonaPlayers();
}

// 0x00106ec0
void GameManagerImpl::AddPersonaPlayers() {
    const char *colors[] = {"green", "purple", "yellow", "red"};
    const int nCount = mPersonas.size();
    std::vector<int> order(nCount);
    for (int i = 0; i < nCount; ++i) {
        order[i] = i;
    }
    for (int i = nCount - 1; i > 0; --i) {
        std::swap(order[i], order[RandomInt(0, i + 1)]);
    }
    for (auto it = mPersonas.begin(); it != mPersonas.end(); ++it) {
        const int nIndex = it - mPersonas.begin();
        mpWorld->AddLocalPlayer(nIndex, nIndex, order[nIndex], HxStr(colors[nIndex]), *it);
    }
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
