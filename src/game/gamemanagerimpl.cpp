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
#include "sch/tick.h"
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

// NTSC-U/C: 0x0010bec8, PAL: 0x0010c060
int GameManagerImpl::CheckState() {
    // Yes, the binary branches on the state and then returns 1 either way. The instruction that
    // looks like the taken path is the branch-likely delay slot.
    if (mState != 0) {
        return 1;
    }
    return 1;
}

// NTSC-U/C: 0x0010b8f0, PAL: 0x0010ba78
void GameManagerImpl::RunStateCheck() {
    CheckState(); // Yes, the binary discards this call's result.
}

// NTSC-U/C: 0x0010c050, PAL: 0x0010c208
void GameManagerImpl::DestroyWorld() {
    Application::shared()->GetWatchdog()->Snapshot();
    mpPoller->DetachController(mpWorld);
    delete mpWorld;
    mpWorld = nullptr;
}

// NTSC-U/C: 0x0010b870, PAL: 0x0010b9f8
int GameManagerImpl::GetWorldLoadFlag() {
    return mWorldLoadFlag;
}

// NTSC-U/C: 0x00105e80, PAL: 0x00105e80
void GameManagerImpl::AddPersona(const MetPersonaData &persona) {
    MetPersonaData *pPersona = new MetPersonaData;
    *pPersona = persona;
    mPersonas.push_back(pPersona);
}

// NTSC-U/C: 0x0010b888, PAL: 0x0010ba10
std::vector<MetPersonaData *> *GameManagerImpl::GetPersonas() {
    return &mPersonas;
}

// NTSC-U/C: 0x0010be20, PAL: 0x0010bfb8
void GameManagerImpl::ClearPersonas() {
    for (std::vector<MetPersonaData *>::iterator it = mPersonas.begin(); it != mPersonas.end();
         ++it) {
        delete *it;
        *it = nullptr;
    }
    mPersonas.erase(mPersonas.begin(), mPersonas.end());
}

// NTSC-U/C: 0x0010b890, PAL: 0x0010ba18
GrooveWorld *GameManagerImpl::GetWorld() {
    return mpWorld;
}

// NTSC-U/C: 0x0010b898, PAL: 0x0010ba20
MetaGameWorld *GameManagerImpl::GetMetaWorld() {
    return mpMetaWorld;
}

// NTSC-U/C: 0x0010b8a0, PAL: 0x0010ba28
InputPoller *GameManagerImpl::GetPoller() {
    return mpPoller;
}

// NTSC-U/C: 0x0010b8a8, PAL: 0x0010ba30
int GameManagerImpl::GetUnwrittenValue() {
    return mUnwrittenValue;
}

// NTSC-U/C: 0x0010b8b0, PAL: 0x0010ba38
GameStats *GameManagerImpl::GetStats() {
    return &mStats;
}

// NTSC-U/C: 0x0010c588, PAL: 0x0010c740
void GameManagerImpl::Save(OBStream *pStream) {
    pStream->WriteLE(&mState, sizeof(mState))
        .WriteLE(&mSavedWord, sizeof(mSavedWord))
        .WriteLE(&mGameMode, sizeof(mGameMode));
    mParams.Save(pStream);
}

// NTSC-U/C: 0x0010b8b8, PAL: 0x0010ba40
int GameManagerImpl::IsPlaybackActive() {
    return mpPlayback != nullptr;
}

// NTSC-U/C: 0x0010c290, PAL: 0x0010c448
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

// NTSC-U/C: 0x0010b8c8, PAL: 0x0010ba50
int GameManagerImpl::GetGameMode() {
    return mGameMode;
}

// NTSC-U/C: 0x0010b8d0, PAL: 0x0010ba58
GameParams *GameManagerImpl::GetParams() {
    return &mParams;
}

// NTSC-U/C: 0x0010b8d8, PAL: 0x0010ba60
int GameManagerImpl::GetChangeCount() {
    return mChangeCount;
}

// NTSC-U/C: 0x0010c210, PAL: 0x0010c3c8
void GameManagerImpl::SetParams(const GameParams &params) {
    CheckState(); // Yes, the binary discards this call's result.
    mParams = params;
    // The two modes are republished from the settings just copied in, not from the argument.
    SetPlayMode(mParams.mPlayMode);
    SetDifficulty(mParams.mDifficulty);
    ++mChangeCount;
    CheckState(); // Yes, the binary discards this call's result.
}

// NTSC-U/C: 0x0010c3e0, PAL: 0x0010c598
void GameManagerImpl::SetDifficulty(int nDifficulty) {
    mParams.mDifficulty = nDifficulty;
    // No literal maps the value, and the raw word goes out as the template argument.
    CallScriptTemplate(kScriptTemplateDifficulty, nDifficulty);
    ++mChangeCount;
}

// NTSC-U/C: 0x0010c348, PAL: 0x0010c500
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

// NTSC-U/C: 0x0010b8e0, PAL: 0x0010ba68
int GameManagerImpl::GetDifficulty() {
    return mParams.mDifficulty;
}

// NTSC-U/C: 0x0010b8e8, PAL: 0x0010ba70
int GameManagerImpl::GetPlayMode() {
    return mParams.mPlayMode;
}

// NTSC-U/C: 0x0010b878, PAL: 0x0010ba00
void GameManagerImpl::SetDrawEnabled(int nEnabled) {
    // Yes, the binary inverts the low bit rather than the whole value, so 2 records 3.
    mDrawSuppressed = nEnabled ^ 1;
}

// NTSC-U/C: 0x00107540, PAL: 0x00107610
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

// NTSC-U/C: 0x00105f50, PAL: 0x00105f50
GameManagerImpl::GameManagerImpl()
    : mState(0), mSavedWord(0), mpWorld(nullptr), mpMetaWorld(nullptr), mUnwrittenValue(0),
      mGameMode(0), mChangeCount(0), mFrontEndActive(0), mpRecorder(nullptr), mpPlayback(nullptr),
      mWorldLoadFlag(1), mRestartPending(0), mPaused(0), mDrawSuppressed(1) {
    mQueue.AddSink(this);
    mpPoller = new InputPoller;
    mpPoller->ClearUnusedFlag();
    CheckState(); // Yes, the binary discards this call's result.
}

// NTSC-U/C: 0x001062d0, PAL: 0x00106310
GameManagerImpl::~GameManagerImpl() {
    CheckState(); // Yes, the binary discards this call's result.
    delete mpRecorder;
    mpRecorder = nullptr;
    delete mpMetaWorld;
    mpMetaWorld = nullptr;
    delete mpPoller;
    mpPoller = nullptr;
}

// NTSC-U/C: 0x001068a0, PAL: 0x00106918
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

// NTSC-U/C: 0x0010c168, PAL: 0x0010c320
void GameManagerImpl::Start() {
    mpMetaWorld = new MetaGameWorld;
    mpPoller->SetController(mpMetaWorld);
    mFrontEndActive = 1;
    mpPoller->SetActive(1);
}

// NTSC-U/C: 0x001069a8, PAL: 0x00106a38
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

// NTSC-U/C: 0x0010c148, PAL: 0x0010c300
void GameManagerImpl::OnEndGame(Message *pMsg) {
    EndGame(static_cast<EndGameMsg *>(pMsg)->mRestart);
}

// NTSC-U/C: 0x00106c08, PAL: 0x00106c98
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

// NTSC-U/C: 0x00106af8, PAL: 0x00106b88
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

// NTSC-U/C: 0x0010c0c0, PAL: 0x0010c278
void GameManagerImpl::FinishWorldLoad() {
    mWorldLoadFlag = 1;
    while (mpWorld->IsLoadDone() == 0) {
    }
    mpWorld->FinishLoad();
    AddPlayers();
    mpWorld->PrepareLevel();
    mpPoller->SetController(mpWorld);
}

// NTSC-U/C: 0x001065a8, PAL: 0x00106620
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

// NTSC-U/C: 0x0010bfa0, PAL: 0x0010c158
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

// NTSC-U/C: 0x00106e28, PAL: 0x00106eb8
void GameManagerImpl::PollPlayback() {
    mpPoller->Poll();
    Application::shared()->GetWatchdog(); // Yes, the binary discards this call's result.
    GetElapsedMilliseconds();             // Yes, the binary discards the reading.
    if (mpPoller->GetPressedThisPoll() != 0 && mpPlayback != nullptr && mpWorld != nullptr) {
        mpWorld->PostFinish();
    }
}

// NTSC-U/C: 0x00106720, PAL: 0x00106798
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
    const Sch::Tick now{0};
    Application::shared()->GetWatchdogTimer()->PostIn(pCommand, now, id, kRecordable);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
    CheckState(); // Yes, the binary discards this call's result.
}

// NTSC-U/C: 0x001072b0, PAL: 0x00107360
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

// NTSC-U/C: 0x0010c128, PAL: 0x0010c2e0
void GameManagerImpl::StartPlay() {
    mpWorld->StartPlay();
}

// NTSC-U/C: 0x0010c1f0, PAL: 0x0010c3a8
void GameManagerImpl::AddPlayers() {
    AddPersonaPlayers();
}

// NTSC-U/C: 0x00106ec0, PAL: 0x00106f50
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

// NTSC-U/C: 0x0010c420, PAL: 0x0010c5d8
void GameManagerImpl::StartRecording() {
    if (mpRecorder != nullptr) {
        Fatal(kRecordingInProgress);
    }
    if (mState != 0) {
        Fatal(kCannotStartRecording);
    }
    mpRecorder = new GameRecorder(this);
}

// NTSC-U/C: 0x0010c4b8, PAL: 0x0010c670
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

// NTSC-U/C: 0x0010bee0, PAL: 0x0010c078
void GameManagerImpl::QueueMessage(Message *pMsg) {
    MsgSink *pQueueSink = &mQueue;
    pQueueSink->DispatchPriv(pMsg);
}

// NTSC-U/C: 0x0010bf10, PAL: 0x0010c0a8
void GameManagerImpl::OnDoPlayback(Message *) {
    HxStr file = QueryConfigString(kPlaybackFileConfigCode);
    Recreate(file, 0);
}
