#include "game/gamemanagerimpl.h"

#include <utility>
#include <vector>

#include "app/application.h"
#include "app/renderer.h"
#include "app/rendererbase.h"
#include "app/scheduler.h"
#include "app/timeclock.h"
#include "game/dogamesystemplaycmd.h"
#include "game/forcefeedbackmgr.h"
#include "game/gameplaybacker.h"
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
#include "sch/time.h"
#include "script/configquery.h"
#include "script/scripthost.h"
#include "synth/ps2hardsynth.h"

namespace {

// Script templates the manager publishes its settings through.
constexpr int kScriptTemplateGameMode = 610;
constexpr int kScriptTemplatePlayMode = 611;
constexpr int kScriptTemplateDifficulty = 612;
constexpr int kScriptTemplateLevelName = 631;
constexpr int kScriptTemplateArenaName = 635;

// Configuration code of the container name CreateWorld() hands the world.
constexpr int kContainerConfigCode = 910;

// The diagnostics StartRecording() and Recreate() trip.
constexpr char kRecordingInProgress[] = "Recording already in progress";
constexpr char kCannotStartRecording[] = "Cannot start recording from this state";
constexpr char kCannotRecreateGame[] = "Cannot recreate game from this state";
constexpr char kPlaybackInProgress[] = "Playback already in progress";

// The MIDI message a pause sends: all notes off, controller 123, on the last channel.
constexpr unsigned char kStatusControlChangeChannel16 = 0xbf;
constexpr unsigned char kControllerAllNotesOff = 123;

// Configuration code of the recording OnDoPlayback() replays.
constexpr int kPlaybackFileConfigCode = 618;

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

void GameManagerImpl::RunStateCheck() {
    CheckState(); // Yes, the binary discards this call's result.
}

void GameManagerImpl::DestroyWorld() {
    Application::shared()->GetWatchdog()->Snapshot();
    mpPoller->DetachController(mpWorld);
    delete mpWorld;
    mpWorld = nullptr;
}

int GameManagerImpl::GetWorldLoadFlag() {
    return mWorldLoadFlag;
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

int GameManagerImpl::GetUnwrittenValue() {
    return mUnwrittenValue;
}

GameStats *GameManagerImpl::GetStats() {
    return &mStats;
}

void GameManagerImpl::Save(OBStream *pStream) {
    pStream->WriteLE(&mState, sizeof(mState))
        .WriteLE(&mSavedWord, sizeof(mSavedWord))
        .WriteLE(&mGameMode, sizeof(mGameMode));
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
    mParams.mNetGame = nMode == kGameModeNet;
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
    SetPlayMode(mParams.mPlayMode);
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
    mParams.mPlayMode = nMode;
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
    return mParams.mPlayMode;
}

void GameManagerImpl::SetDrawEnabled(int nEnabled) {
    // Yes, the binary inverts the low bit rather than the whole value, so 2 records 3.
    mDrawSuppressed = nEnabled ^ 1;
}

void GameManagerImpl::DispatchPriv(Message *pMsg) {
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
        Fatal("DISPATCH_CHECK: Unhandled Message: %s", pMsg->GetName());
    }
}

GameManagerImpl::GameManagerImpl()
    : mState(0), mSavedWord(0), mpWorld(nullptr), mpMetaWorld(nullptr), mUnwrittenValue(0),
      mGameMode(0), mChangeCount(0), mFrontEndActive(0), mpRecorder(nullptr), mpPlayback(nullptr),
      mWorldLoadFlag(1), mRestartPending(0), mPaused(0), mDrawSuppressed(1) {
    mQueue.AddSink(this);
    mpPoller = new InputPoller;
    mpPoller->ClearUnusedFlag();
    CheckState(); // Yes, the binary discards this call's result.
}

GameManagerImpl::~GameManagerImpl() {
    CheckState(); // Yes, the binary discards this call's result.
    delete mpRecorder;
    mpRecorder = nullptr;
    delete mpMetaWorld;
    mpMetaWorld = nullptr;
    delete mpPoller;
    mpPoller = nullptr;
}

void GameManagerImpl::CreateWorld() {
    mpWorld = new GrooveWorld(Application::shared(), &mStats);
    const HxStr &level = mParams.mLevelName;
    CallScriptTemplate(kScriptTemplateLevelName,
                       level.mStr != nullptr ? level.mStr : g_szEmptyString);
    const HxStr &arena = mParams.mArenaName;
    CallScriptTemplate(kScriptTemplateArenaName,
                       arena.mStr != nullptr ? arena.mStr : g_szEmptyString);

    HxStr container = QueryConfigString(kContainerConfigCode);
    mpWorld->StartLoad(container);
}

void GameManagerImpl::Start() {
    mpMetaWorld = new MetaGameWorld;
    mpPoller->SetController(mpMetaWorld);
    mFrontEndActive = 1;
    mpPoller->SetActive(1);
}

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
    Application::shared()->GetSynth()->PlayMidi(
        kStatusControlChangeChannel16, kControllerAllNotesOff, 0);
    Application::shared()->GetSynth()->SetPaused(1);
    if (mpWorld != nullptr) {
        mpWorld->mForceFeedback->SetPaused(1);
    }

    MetStartPauseMsg pause;
    mpMetaWorld->GetRenderer()->Dispatch(&pause);
}

void GameManagerImpl::OnEndGame(Message *pMsg) {
    EndGame(static_cast<EndGameMsg *>(pMsg)->mRestart);
}

void GameManagerImpl::EndGame(int bRestart) {
    CheckState(); // Yes, the binary discards this call's result.
    const int nWorldExitFlag = mpWorld->mContinueJukebox;
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
        mRestartPending = 1;
        BeginGameLocalMsg begin;
        QueueMessage(&begin);
    } else {
        mpPoller->SetController(mpMetaWorld);
        mpPoller->SetActive(1);
        mFrontEndActive = 1;
        MetFreqEndedMsg ended;
        ended.mStopJukebox = nWorldExitFlag == 0;
        mpMetaWorld->GetRenderer()->Dispatch(&ended);
    }
    CheckState(); // Yes, the binary discards this call's result.
}

void GameManagerImpl::OnUnpauseGameSystem(Message *) {
    if (mPaused == 0) {
        return;
    }

    mPaused = 0;
    mpMetaWorld->StopFrontEnd();
    mpPoller->SetController(mpWorld);
    mpPoller->SetPaused(0);
    if (GetWorld()->mIsTutorial == 0) {
        GetWorld()->mInputMap->Rebuild();
    }
    Application::shared()->GetSynth()->SetPaused(0);
    if (mpWorld != nullptr) {
        mpWorld->mForceFeedback->SetPaused(0);
    }
    if (GetGameMode() != kGameModeNet) {
        Application::shared()->GetWatchdog()->mClock.Resume();
    }
}

void GameManagerImpl::FinishWorldLoad() {
    mWorldLoadFlag = 1;
    while (mpWorld->IsLoadDone() == 0) {
    }
    mpWorld->FinishLoad();
    AddPlayers();
    mpWorld->PrepareLevel();
    mpPoller->SetController(mpWorld);
}

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
        roots[i]->PollMessages();
        roots[i]->Update();
    }
    if (mFrontEndActive != 0) {
        MemcardManager::shared()->Update();
    }
    if (mDrawSuppressed != 0) {
        return;
    }

    Rnd::ThePs.BeginFrame();
    Rnd::ThePs.EnterVu1Path();
    for (int i = 0; i < nRootCount; ++i) {
        roots[i]->Draw();
    }
    Rnd::ThePs.LeaveVu1Path();
    Rnd::ThePs.PresentFrame(kNoBufferSwap);
}

void GameManagerImpl::DrawFrameSimple() {
    if (mpMetaWorld == nullptr || mpWorld != nullptr || mDrawSuppressed != 0) {
        return;
    }
    mpMetaWorld->GetRenderer()->UpdateSimple();
    Rnd::ThePs.BeginFrame();
    Rnd::ThePs.EnterVu1Path();
    mpMetaWorld->GetRenderer()->DrawSimple();
    Rnd::ThePs.LeaveVu1Path();
    Rnd::ThePs.PresentFrame(kNoBufferSwap);
}

void GameManagerImpl::PollPlayback() {
    mpPoller->Poll();
    Application::shared()->GetWatchdog(); // Yes, the binary discards this call's result.
    GetElapsedMilliseconds();             // Yes, the binary discards the reading.
    if (mpPoller->GetPressedThisPoll() != 0 && mpPlayback != nullptr && mpWorld != nullptr) {
        mpWorld->PostFinish();
    }
}

void GameManagerImpl::OnBeginGameLocal(Message *) {
    CheckState(); // Yes, the binary discards this call's result.
    if (mRestartPending != 0) {
        mRestartPending = 0;
    } else {
        mpMetaWorld->StopFrontEnd();
    }
    mpPoller->SetActive(0);
    mFrontEndActive = 0;
    {
        IsRecordingMsg recording;
        recording.mIsRecording = 0;
        mpMetaWorld->GetRenderer()->Dispatch(&recording);
    }

    CreateWorld();
    FinishWorldLoad();
    mpWorld->mIsPlayback = 0;
    mpPoller->SetGameInputEnabled(!Application::shared()->IsJukeboxMode());
    if (mpRecorder != nullptr) {
        mpRecorder->BeginRecording(mGameMode, mParams);
    }
    Application::shared()->GetWatchdog()->Flush();

    DoGameSystemPlayCmd *pCommand = new DoGameSystemPlayCmd;
    Sch::CmdID id;
    id.mValue = kUnallocatedCommand;
    const Sch::Time now{0};
    Application::shared()->GetWatchdogTimer()->PostIn(pCommand, now, id, kRecordable);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
    CheckState(); // Yes, the binary discards this call's result.
}

void GameManagerImpl::Load(IBStream *pStream) {
    int nState;
    pStream->ReadLE(&nState, sizeof(nState));
    int nSavedWord;
    pStream->ReadLE(&nSavedWord, sizeof(nSavedWord));
    int nGameMode;
    pStream->ReadLE(&nGameMode, sizeof(nGameMode));
    mParams.Load(pStream);
    mState = nState;
    mSavedWord = nSavedWord;
    mGameMode = nGameMode;
    SetGameMode(nGameMode);
    SetPlayMode(mParams.mPlayMode);
    SetDifficulty(mParams.mDifficulty);

    ClearPersonas();
    MetPersonaData persona;
    persona.mAppearance.mUserName = HxStr("freq player 1");
    AddPersona(persona);
    {
        IsRecordingMsg recording;
        recording.mIsRecording = 1;
        mpMetaWorld->GetRenderer()->Dispatch(&recording);
    }

    Renderer::LoadLevel(mParams);
    CreateWorld();
    FinishWorldLoad(); // The binary expands this body inline here.
    mpWorld->mIsPlayback = 1;
    mpPoller->SetGameInputEnabled(0);
}

void GameManagerImpl::StartPlay() {
    mpWorld->StartPlay();
}

void GameManagerImpl::AddPlayers() {
    AddPersonaPlayers();
}

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

void GameManagerImpl::StartRecording() {
    if (mpRecorder != nullptr) {
        Fatal(kRecordingInProgress);
    }
    if (mState != 0) {
        Fatal(kCannotStartRecording);
    }
    mpRecorder = new GameRecorder(this);
}

void GameManagerImpl::Recreate(const HxStr &file, int nUnusedFlag) {
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
    mpMetaWorld->StopFrontEnd();
    mpPlayback = new GamePlaybacker(file, this, nUnusedFlag);
}

void GameManagerImpl::QueueMessage(Message *pMsg) {
    MsgSink *pQueueSink = &mQueue;
    pQueueSink->DispatchPriv(pMsg);
}

void GameManagerImpl::OnDoPlayback(Message *) {
    HxStr file = QueryConfigString(kPlaybackFileConfigCode);
    Recreate(file, 0);
}
