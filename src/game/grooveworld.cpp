#include "game/grooveworld.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <string.h>
#include <vector>

#include "app/application.h"
#include "app/mainloop.h"
#include "app/playsound.h"
#include "app/renderer.h"
#include "app/watchdog.h"
#include "app/watchdogtimer.h"
#include "game/axingstg.h"
#include "game/bgtrackgraph.h"
#include "game/catchingstg.h"
#include "game/controllercmd.h"
#include "game/delayer.h"
#include "game/forcefeedbackmgr.h"
#include "game/gamemanagerimpl.h"
#include "game/gamer.h"
#include "game/gamestats.h"
#include "game/globalsettings.h"
#include "game/inputcheatdetectorgs.h"
#include "game/inputmap.h"
#include "game/levelbuilder.h"
#include "game/levelconverter.h"
#include "game/leveldata.h"
#include "game/localplayer.h"
#include "game/msgjoiner.h"
#include "game/netplayer.h"
#include "game/phrasedatabase.h"
#include "game/pitchingstg.h"
#include "game/player.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"
#include "game/trackselector.h"
#include "game/voxingstg.h"
#include "gfx/gfxdevice.h"
#include "gs/musesynth.h"
#include "math/color.h"
#include "met/metpersonadata.h"
#include "met/metremixrecord.h"
#include "mid/mbt.h"
#include "msg/bumppacket.h"
#include "msg/cripplepacket.h"
#include "msg/endgamemsg.h"
#include "msg/fadegamemsg.h"
#include "msg/gamebeginmsg.h"
#include "msg/gameovermsg.h"
#include "msg/message.h"
#include "msg/metcontrollerreading.h"
#include "msg/pausegamesystemmsg.h"
#include "msg/rawcontrollermsg.h"
#include "msg/seekermsg.h"
#include "msg/textmsg.h"
#include "msg/trackselectmsg.h"
#include "os/async.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"
#include "sch/command.h"
#include "sch/tick.h"
#include "sch/tickclock.h"
#include "script/configquery.h"
#include "script/scripthost.h"
#include "stream/ibstream.h"
#include "stream/iobpreallocmemstream.h"
#include "stream/obstream.h"
#include "synth/midi_main.h"
#include "synth/ps2hardsynth.h"

namespace {

// Configuration identifiers the load path queries.
constexpr int kTrackCountQuery = 0x384;
constexpr int kLevelConverterOptionQuery = 0x39a;
// One more than the track a solo player takes.
constexpr int kSoloTrackConfigCode = 0x3a6;

// Set while the level plays streamed audio. FinishSong() then stops the sound-bank movie.
constexpr int kStreamedAudioQuery = 0x3a4;
// The level name FinishSong() records in the log.
constexpr int kLevelNameQuery = 0x278;

// mState values.
constexpr int kStateLoading = 1;
constexpr int kStateLoaded = 2;
constexpr int kStateEnded = 6;
// What PrepareLevel() leaves in mState once the level is ready to start, and what StartPlay()
// leaves once the world accepts controller readings.
constexpr int kStatePrepared = 3;
constexpr int kStatePlaying = 4;
// What PostExit() leaves in mState once an exit is under way.
constexpr int kStateExiting = 5;

// The handle PostExit() passes before the scheduler allocates one, and the recordable flag.
constexpr int kUnallocatedCommand = -2;
constexpr int kRecordable = 1;

// The fade Exit() applies, in milliseconds: the default, the length for exit mode 1 or a running
// playback, and the length in jukebox mode. The screen fade runs 500 milliseconds longer, and
// FinishSong() runs 600 milliseconds after the fade.
constexpr int kExitFadeMs = 1000;
constexpr int kExitFadeLongMs = 3000;
constexpr int kExitFadeJukeboxMs = 5000;
constexpr int kExitScreenFadeExtraMs = 500;
constexpr int kExitFinishDelayMs = 600;
constexpr long long kNsPerMs = 1000000;
constexpr int kFadeOut = 0;

// The joystick tag of a controller reading, the button OnUnknownSlot2() treats as pause, and the
// bound below which a joystick button counts during a playback.
constexpr int kReadingTypeJoy = 0x6a6f7920;
constexpr int kPauseButton = 10;
constexpr int kJoyButtonLimit = 100;

// The fade StartPlay() sends to the delayer, the track and bar whose quantum sets when input is
// enabled, and the lead of the metronome it starts, in MIDI ticks.
constexpr int kStartFadeMs = 1000;
constexpr int kStartFadeIn = 1;
constexpr int kFirstTrack = 0;
constexpr int kFirstBar = 0;
constexpr int kMetronomeLeadTicks = 3200;

// The configuration codes PrepareLevel() reads: the sound-bank movie flag and its path, the start
// offset in ticks, and the flag it stores in mUnknown90.
constexpr int kSoundBankMovieFlagCode = 0x3a4;
constexpr int kSoundBankMoviePathCode = 0x3a5;
constexpr int kStartOffsetCode = 0x38d;
constexpr int kUnknown90FlagCode = 0x3a1;

// The Slot13() argument FinishSong() passes to the synthesiser.
constexpr int kSynthSlot13Off = 0;

// Player::Slot2() while the player has no seeker.
constexpr int kNoSeeker = -1;

// Bytes of each text field of the log record FinishSong() writes and BuildGraphs() reads.
constexpr int kLogTextLength = 32;
// The byte written on each side of the two text fields.
constexpr char kLogMarker = 1;

// The script template BuildGraphs() runs with the player count.
constexpr int kPlayerCountTemplate = 0x266;

// BGTrackGraph's second constructor argument for a backing and for an intro track.
constexpr int kBackingTrackGraph = 0;
constexpr int kIntroTrackGraph = 1;

// IBStream::Seek() origin that measures from the start of the buffer.
constexpr int kSeekFromStart = 0;

// The delay before exit mode 2 runs EndLevel(), in nanoseconds.
constexpr long long kEndLevelDelayNs = 500000000;

// The clear colour exit mode 3 leaves on the display.
constexpr float kOpaque = 1.0f;

// AsyncPollComplete() results.
constexpr int kAsyncComplete = 0;
constexpr int kAsyncPending = -1;

// The tag and line FinishLoad() bills the release of the file buffer to.
constexpr char kAllocTag[] = "GrooveWorld.cpp";
constexpr int kAllocLine = 302;

// Exit modes PostExitMode1(), PostExitMode2(), and PostExitMode3() queue.
constexpr int kExitMode1 = 1;
constexpr int kExitMode2 = 2;
constexpr int kExitMode3 = 3;

/**
 * Scheduler command that calls one member of the world.
 *
 * `Q234_GLOBAL_$N$_13ControllerCmd$sCmdID7FuncCmd` in the RTTI, with Sch::Command as its one base.
 * The anonymous-namespace marker records this translation unit through its first global,
 * ControllerCmd::sCmdID. Its vtable at `0x007dc378` retains Sch::Command::Save() and Load(). The
 * GrooveWorld routines that queue one allocate 0x18 bytes and expand the constructor inline,
 * storing the world at `+0x0c` and an eight-byte pointer to member function at `+0x10`.
 *
 * The destructor at `0x001947d8` is implicitly declared. It stores the base table pointer and runs
 * Attachment's destructor, which is what the compiler generates.
 */
class FuncCmd : public Sch::Command {
public:
    FuncCmd(GrooveWorld *pWorld, void (GrooveWorld::*pfnFunc)()) : mWorld(pWorld), mFunc(pfnFunc) {
    }

    // 0x00194850
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x00194860
    virtual void Execute() {
        (mWorld->*mFunc)();
    }

    // 0x001948e0
    virtual void Print(std::ostream &stream) {
        stream << "{GWFunc}";
    }

    // The word at 0x0067f244, which the image initialises to zero.
    static int sCmdID;

private:
    GrooveWorld *mWorld;          // +0x0c
    void (GrooveWorld::*mFunc)(); // +0x10
};

int FuncCmd::sCmdID;

/**
 * Scheduler command that makes the world leave the game.
 *
 * `Q234_GLOBAL_$N$_13ControllerCmd$sCmdID7ExitCmd` in the RTTI, with Sch::Command as its one base,
 * in the same translation unit as FuncCmd. Its vtable at `0x007dc330` overrides every slot the
 * base declares apart from Attachment::Destroy(). Both constructors have out-of-line copies and no
 * caller in the image, and the GrooveWorld routine at `0x0018e368` expands the second inline.
 *
 * The destructor at `0x00194910` is implicitly declared, for the reason recorded on FuncCmd.
 */
class ExitCmd : public Sch::Command {
public:
    // 0x00194998
    ExitCmd() {
    }

    // 0x001949b8
    ExitCmd(int nMode, int nUnknown10, int nUnknown14)
        : mMode(nMode), mUnknown10(nUnknown10), mUnknown14(nUnknown14) {
    }

    // 0x00194988
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001949e8
    virtual void Execute() {
        Application::shared()->GetWorld()->Exit(mMode, mUnknown10, mUnknown14);
    }

    // 0x00194a28
    virtual void Print(std::ostream &stream) {
        stream << "{ExitCmd}";
    }

    // 0x00194a50
    virtual void Save(OBStream &stream) {
        const int nMode = mMode;
        stream.Write(&nMode, sizeof(nMode));
        stream << mUnknown10;
        stream << mUnknown14;
    }

    // 0x00194ab8
    virtual void Load(IBStream &stream) {
        int nMode;
        stream.Read(&nMode, sizeof(nMode));
        mMode = nMode;
        stream >> mUnknown10;
        stream >> mUnknown14;
    }

    // 0x0018beb0
    // The factory the unit's static initialiser registers. The expanded default constructor sets
    // the reference count to 1 and leaves the three members unwritten.
    static Sch::Command *NewCmd() {
        return new ExitCmd;
    }

    // The word at 0x0067f24c, which the image initialises to 7.
    static int sCmdID;

private:
    int mMode;      // +0x0c
    int mUnknown10; // +0x10, one byte on the wire
    int mUnknown14; // +0x14, one byte on the wire
};

constexpr int kExitCmdId = 7;

int ExitCmd::sCmdID = kExitCmdId;

} // namespace

// 0x0018bef0
GrooveWorld::GrooveWorld(Application *pApp, GameStats *pStats)
    : mApp(pApp), mInputMap(nullptr), mTrackSelector(nullptr), mJoiner(nullptr), mLevel(nullptr),
      mUnknown1c(nullptr), mUnknown20(nullptr), mDelayer(nullptr), mRenderer(nullptr),
      mGamer(nullptr), mStats(pStats), mUnknown5c(nullptr), mMuseSynth(nullptr),
      mSongClock(nullptr), mCheatDetector(nullptr), mUnknown84(0), mUnknown88(0), mUnknown8c(0),
      mUnknown90(0), mUnknown94(0), mState(0), mUnknownb8(1) {
    mSongClock = new Sch::TickClock(mApp->GetWatchdog(), nullptr);
    mCheatDetector = new InputCheatDetectorGS(&g_gameCheatSequences);
    mForceFeedback = new ForceFeedbackMgr;
}

// 0x0018c368
GrooveWorld::~GrooveWorld() {
    Shutdown();
}

// 0x001951e8
void GrooveWorld::Shutdown() {
    if (mState == kStateEnded) {
        StopLevel();
    }
    DeletePlayers();
    delete mLevel;
    delete mSongClock;
    delete mCheatDetector;
    delete mForceFeedback;
    StopNoteDestroyer();
    DestroyNoteDestroyer();
}

// 0x0018dc88
void GrooveWorld::PrepareLevel() {
    Ps2HardSynth *pSynth = mApp->GetSynth();
    pSynth->LoadBankSet5();
    pSynth->LoadBankSet6();
    pSynth->Slot13(Application::shared()->GetPlayMode() == kPlayModeJam);
    BuildGraphs();
    CreateRenderer();
    ConnectPlayers();
    if (QueryConfigFlag(kSoundBankMovieFlagCode) != 0) {
        HxStr path = QueryConfigString(kSoundBankMoviePathCode);
        StartSoundBankMovie(path.mStr != nullptr ? path.mStr : g_szEmptyString);
    }
    const Mid::MBT offset(QueryConfigValue(kStartOffsetCode));
    const Mid::MBT zero(0);
    const Mid::MBT start(std::min(kMBTMaximum, std::max(kMBTMinimum, zero.mTick - offset.mTick)));
    mSongClock->SetSongTick(start);
    mUnknown90 = QueryConfigFlag(kUnknown90FlagCode);
    mState = kStatePrepared;
}

// 0x0018de38
void GrooveWorld::StartPlay() {
    mInputMap->DisableEntries();
    mApp->GetWatchdog()->Flush();
    mSongClock->Resume();
    CreateNoteDestroyer();
    StartNoteDestroyer();
    mState = kStatePlaying;
    mApp->GetSynth()->Slot10();
    std::for_each(
        mUnknown50.begin(), mUnknown50.end(), std::mem_fn(&BGTrackGraph::CallBuildSequencer));

    if (mUnknown90 == 0) {
        FuncCmd *pEnable = new FuncCmd(this, &GrooveWorld::EnableInput);
        const Mid::MBT zero(0);
        const Mid::MBT lead(mLevel->GetTrack(kFirstTrack)->GetQuant(kFirstBar) / 2);
        const Mid::MBT when(std::min(kMBTMaximum, std::max(kMBTMinimum, zero.mTick - lead.mTick)));
        mSongClock->PostAtSongTick(pEnable, when.mTick);
        Attachment::ReleaseIfSet(pEnable);
    }

    FuncCmd *pStart = new FuncCmd(this, &GrooveWorld::StartSequencers);
    mSongClock->PostAtSongTick(pStart, Mid::MBT(0).mTick);
    Attachment::ReleaseIfSet(pStart);

    GameBeginMsg begin;
    mDelayer->Handle(&begin);
    mJoiner->Handle(&begin);
    FadeGameMsg fade;
    fade.mDuration = kStartFadeMs;
    fade.mFadeIn = kStartFadeIn;
    mDelayer->Handle(&fade);

    mStats->Reset(mPlayers.size());
    std::for_each(mPlayers.begin(), mPlayers.end(), std::mem_fn(&Player::CallSlot11));
    mForceFeedback->SetJukeboxMode(Application::shared()->IsJukeboxMode());
    mForceFeedback->SetUnknownFlag04(mUnknown8c);
    mForceFeedback->SetEnabled(GlobalSettings::shared()->mGameOptions.mUnknown08);
    mForceFeedback->SetPlayerCount(mLocalPlayers.size());
    mForceFeedback->StartMetronome(Mid::MBT(kMetronomeLeadTicks));
}

// 0x0018ed98
void GrooveWorld::OnUnknownSlot2(int nUnknown1, int nUnknown2, int nUnknown3, float flUnknown4) {
    if (mState != kStatePlaying) {
        return;
    }
    if (mUnknown90 == 0) {
        mCheatDetector->OnUnknownSlot2(nUnknown1, nUnknown2, nUnknown3, flUnknown4);
    }
    if (mApp->IsJukeboxMode()) {
        if (nUnknown1 == kReadingTypeJoy && flUnknown4 > 0.0f && nUnknown3 == kPauseButton) {
            mUnknownb8 = 0;
            PostExit(kExitMode1, mUnknownb8, 0);
        }
        return;
    }
    if (mApp->GetGameManager()->IsPlaybackActive() == 1) {
        if (nUnknown1 == kReadingTypeJoy && flUnknown4 > 0.0f && nUnknown3 < kJoyButtonLimit) {
            PostExit(kExitMode1, mUnknownb8, 0);
            return;
        }
    } else if (nUnknown1 == kReadingTypeJoy && nUnknown3 == kPauseButton && flUnknown4 > 0.0f &&
               !(mLocalPlayers.size() < static_cast<unsigned>(nUnknown2))) {
        const int nGameMode = Application::shared()->GetGameMode();
        if (nGameMode == kGameModeSolo && Application::shared()->GetPlayMode() == nGameMode &&
            mStats->mCompleted != 0) {
            PostExit(kExitMode1, mUnknownb8, 0);
        } else {
            PauseGameSystemMsg pause;
            mApp->GetGameManager()->QueueMessage(&pause);
            mInputMap->StopAllRiffs();
        }
        return;
    } else if (mUnknown84 != 0) {
        return;
    }
    const MetControllerReading reading{nUnknown1, nUnknown2, nUnknown3, flUnknown4};
    ControllerCmd *pCommand = new ControllerCmd(reading);
    CmdID id;
    id.mValue = kUnallocatedCommand;
    mApp->GetWatchdogTimer()->PostIn(pCommand, Sch::Tick{0}, id, kRecordable);
    Attachment::ReleaseIfSet(pCommand);
}

// 0x0018f078
void GrooveWorld::ReplayControllerReading(const MetControllerReading *pReading) {
    if (mInputMap == nullptr || mState != kStatePlaying) {
        return;
    }
    RawControllerMsg msg;
    msg.mReading = *pReading;
    // Yes, the binary stores the tick without the finite check an Mid::MBT constructor runs.
    msg.mPosition.mTick = mSongClock->SongTick();
    mInputMap->Handle(&msg);
}

// 0x0018e368
void GrooveWorld::PostExit(int nMode, int nUnknownb8, int nUnknown88) {
    if (mState != kStatePlaying) {
        return;
    }
    mState = kStateExiting;
    if (mApp->GetGameManager()->IsPlaybackActive() != 0 || nUnknown88 != 0) {
        Exit(nMode, nUnknownb8, nUnknown88);
        return;
    }
    ExitCmd *pCommand = new ExitCmd(nMode, nUnknownb8, nUnknown88);
    CmdID id;
    id.mValue = kUnallocatedCommand;
    Application::shared()->GetWatchdogTimer()->PostIn(pCommand, Sch::Tick{0}, id, kRecordable);
    Attachment::ReleaseIfSet(pCommand);
}

// 0x0018e478
void GrooveWorld::Exit(int nMode, int nUnknownb8, int nUnknown88) {
    mUnknown94 = nMode;
    mUnknownb8 = nUnknownb8;
    mUnknown88 = nUnknown88;
    mInputMap->StopAllRiffs();
    mInputMap->DisableEntries();
    GameOverMsg over;
    mDelayer->Handle(&over);

    int bFadeSynth = 0;
    if (mUnknown94 == kExitMode1 || mApp->GetGameManager()->IsPlaybackActive() != 0) {
        bFadeSynth = 1;
    }
    int nFadeMs = kExitFadeMs;
    if (mApp->IsJukeboxMode()) {
        nFadeMs = kExitFadeJukeboxMs;
    } else if (bFadeSynth != 0) {
        nFadeMs = kExitFadeLongMs;
    }
    FadeGameMsg fade;
    fade.mDuration = nFadeMs + kExitScreenFadeExtraMs;
    fade.mFadeIn = kFadeOut;
    mDelayer->Handle(&fade);

    if (bFadeSynth != 0) {
        Application::shared()->GetSynth()->FadeOut(nFadeMs);
    } else {
        Application::shared()->GetSynth()->AllNotesOffExceptSfxChannel();
        Application::shared()->GetWatchdog()->Snapshot();
        mSongClock->Pause();
    }
    mForceFeedback->StopAll(Mid::MBT(0));

    FuncCmd *pFinish = new FuncCmd(this, &GrooveWorld::FinishSong);
    [[maybe_unused]] CmdID id;
    id.mValue = kUnallocatedCommand; // Yes, the binary prepares this handle and never passes it.
    mApp->GetWatchdogTimer()->PostIn(
        pFinish, Sch::Tick{static_cast<long long>(nFadeMs + kExitFinishDelayMs) * kNsPerMs});
    Attachment::ReleaseIfSet(pFinish);
}

// 0x00195388
void GrooveWorld::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nCripplePacketType) {
        OnCripplePacket(pMsg);
    } else if (nType == g_nBumpPacketType) {
        OnBumpPacket(pMsg);
    }
}

// 0x00195348
void GrooveWorld::OnCripplePacket(Message *pMsg) {
    mDelayer->Handle(pMsg);
}

// 0x00194b50
void GrooveWorld::OnBumpPacket(Message *pMsg) {
    mTrackSelector->Handle(pMsg);
}

// 0x00194b80
void GrooveWorld::SetUnknown20And1c(MsgSink *pSink, MsgSource *pSource) {
    mUnknown20 = pSink;
    if (mApp->GetGameMode() == kGameModeNet) {
        mUnknown1c = pSource;
    } else {
        mUnknown1c = nullptr;
    }
}

// 0x00194bc8
void GrooveWorld::StartLoad(const HxStr &path) {
    mLevel = new LevelBuilder(QueryConfigValue(kTrackCountQuery));
    mLevelPath = path;

    const int nZone = ZoneGetCurrent();
    ZoneSetCurrent(kNoZone);
    mLoadHandle = AsyncLoadFileByPath(
        path.mStr != nullptr ? path.mStr : g_szEmptyString, nullptr, 0, nullptr);
    ZoneSetCurrent(nZone);

    mState = kStateLoading;
}

// 0x00194ca0
int GrooveWorld::IsLoadDone() {
    AsyncPumpCompletedRequests();
    const int nResult = AsyncPollComplete(mLoadHandle, &mLoadBuffer, &mLoadSize);
    if (nResult == kAsyncComplete) {
        return 1;
    }
    if (nResult == kAsyncPending) {
        return 0;
    }
    Fatal("Error reading midi file asynchronously\n");
    return 0;
}

// 0x00194d00
void GrooveWorld::FinishLoad() {
    LevelConverter converter;
    if (QueryConfigFlag(kLevelConverterOptionQuery) != 0) {
        converter.mUnknown90 = 1;
    }
    converter.Convert(mLevelPath.mStr != nullptr ? mLevelPath.mStr : g_szEmptyString,
                      mLoadBuffer,
                      mLoadSize,
                      mLevel);
    MemFreeTagged(mLoadBuffer, kAllocTag, kAllocLine);

    mSongClock->SetTempoMap(mLevel->OnUnknownSlot7());

    mLoadHandle = 0;
    mState = kStateLoaded;
    mLoadBuffer = nullptr;
    mLoadSize = 0;
}

// 0x0018c828
void GrooveWorld::ConnectPlayers() {
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        Player *pPlayer = *it;
        mInputMap->AddSink(pPlayer);
        mTrackSelector->AddSink(pPlayer);
        if (mUnknown1c != nullptr) {
            mUnknown1c->AddSink(pPlayer);
        }
        pPlayer->AddSink(mJoiner);
        pPlayer->AddSink(mGamer);
        pPlayer->AddSink(mTrackSelector);
        if (mUnknown20 != nullptr) {
            pPlayer->AddSink(mUnknown20);
        }
    }
}

// 0x0018c960
void GrooveWorld::DisconnectPlayers() {
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        Player *pPlayer = *it;
        if (mUnknown20 != nullptr) {
            pPlayer->RemoveSink(mUnknown20);
        }
        pPlayer->RemoveSink(mGamer);
        pPlayer->RemoveSink(mJoiner);
        pPlayer->RemoveSink(mTrackSelector);
        if (mUnknown1c != nullptr) {
            mUnknown1c->RemoveSink(pPlayer);
        }
        mTrackSelector->RemoveSink(pPlayer);
        mInputMap->RemoveSink(pPlayer);
    }
}

// 0x0018c778
void GrooveWorld::DeletePlayers() {
    std::for_each(mPlayers.begin(), mPlayers.end(), Player::Delete);
    mPlayers.clear();
    mLocalPlayers.clear();
}

// 0x0018cce8
void GrooveWorld::BuildGraphs() {
    CallScriptTemplate(kPlayerCountTemplate, mPlayers.size());
    if (mLevel == nullptr) {
        Fatal("MIDI level file has not been loaded.");
    }
    mLevel->SetBarCount(0);

    mDelayer = new Delayer;
    mJoiner = new MsgJoiner;
    mInputMap = new InputMap(mApp, &mPlayers);
    mInputMap->mUnknown00 = 1;
    mInputMap->Rebuild();
    mInputMap->AddSink(mJoiner);

    mTrackSelector = new TrackSelector(mPlayers);
    mInputMap->AddSink(mTrackSelector);
    mTrackSelector->AddSink(mJoiner);
    mTrackSelector->AddSink(mDelayer);

    mMuseSynth = new MuseSynth(mSongClock);
    mMuseSynth->AddSink(mApp->GetSynth());
    if (mUnknown1c != nullptr) {
        mUnknown1c->AddSink(this);
    }

    if (mLevel->OwnTrack() != nullptr) {
        mUnknown5c = new BGTrackGraph(0, 0);
        mUnknown5c->CreateMixer(mLevel->OwnTrack());
        mUnknown5c->AddSynthSink(mDelayer);
    }

    mGamer = new Gamer(mLevel->TrackCount(), mLevel->OnUnknownSlot9(), mStats);
    mGamer->AddSink(mDelayer);
    if (mApp->GetGameMode() == kGameModeNet) {
        mGamer->AddSink(mUnknown20);
    }
    mInputMap->AddSink(mGamer);

    for (unsigned int i = 0; i < static_cast<unsigned int>(mLevel->BackingTrackCount()); ++i) {
        BGTrackGraph *pGraph = new BGTrackGraph(i, kBackingTrackGraph);
        mUnknown44.push_back(pGraph);
        pGraph->CreateMixer(mLevel->BackingTrackAt(i));
        pGraph->AttachMixerToSynth(mApp->GetSynth());
        pGraph->AttachMixerToSource(mGamer);
    }
    mGamer->SetBackGraphs(&mUnknown44);

    for (unsigned int i = 0; i < mLevel->mIntroTracks.size(); ++i) {
        BGTrackGraph *pGraph = new BGTrackGraph(i, kIntroTrackGraph);
        mUnknown50.push_back(pGraph);
        pGraph->CreateMixer(mLevel->IntroTrackAt(i));
        pGraph->AttachMixerToSynth(mApp->GetSynth());
    }

    for (unsigned int i = 0; i < static_cast<unsigned int>(mLevel->TrackCount()); ++i) {
        TrackData *pTrack = mLevel->GetTrack(i);
        pTrack->mGamer = mGamer;
        ScoreTrackGraph *pGraph = nullptr; // Yes, the binary leaves it unset on the Fatal path.
        if (pTrack->mKind == kTrackModeCatch ||
            (Application::shared()->GetPlayMode() == kPlayModeGame &&
             pTrack->mKind == kTrackModeRiff &&
             Application::shared()->GetGameManager()->GetParams()->mLoadingGame != 0)) {
            pTrack->mKind = kTrackModeCatch;
            pGraph = new CatchingSTG(pTrack);
        } else if (pTrack->mKind == kTrackModeRiff) {
            pGraph = new PitchingSTG(pTrack);
        } else if (pTrack->mKind == kTrackModeScratch) {
            pGraph = new PitchingSTG(pTrack);
        } else if (pTrack->mKind == kTrackModeAxe) {
            pGraph = new AxingSTG(pTrack);
        } else if (pTrack->mKind == kTrackModeVocal) {
            pGraph = new VoxingSTG(pTrack);
        } else {
            Fatal("Unsupported STG for track %d", pTrack->mUnknown04);
        }
        mTrackGraphs.push_back(pGraph);
        pGraph->Slot4(mJoiner, mUnknown1c, &mGamer->mTrackSources[i]);
        pGraph->Slot5(mGamer);
        pGraph->Slot7(mDelayer);
        pGraph->Slot7(mGamer);
        pGraph->Slot7(mTrackSelector);
        pGraph->Slot6(mApp->GetSynth());
        if (mApp->GetGameMode() == kGameModeNet) {
            pGraph->Slot8(mUnknown20);
        }
    }

    if (Application::shared()->GetGameManager()->GetParams()->mLoadingGame != 0) {
        std::vector<MetRemixRecord> records; // Yes, the binary builds and frees an unused vector.
        char szTitle[kLogTextLength];
        memset(szTitle, 0, sizeof(szTitle));
        char szLevel[kLogTextLength];
        memset(szLevel, 0, sizeof(szLevel));

        IOBPreallocMemStream *pLog = Application::shared()->GetLog();
        pLog->Seek(0, kSeekFromStart);
        int nSize;
        char bLeading;
        char bTrailing;
        pLog->Read(&nSize, sizeof(nSize)).ReadBytes(&bLeading, sizeof(bLeading));
        pLog->ReadBytes(szLevel, sizeof(szLevel));
        pLog->ReadBytes(szTitle, sizeof(szTitle));
        pLog->ReadBytes(&bTrailing, sizeof(bTrailing));
        mSongName = HxStr(szTitle);
        LoadPhrases(*pLog, 0);

        if (Application::shared()->GetPlayMode() == kPlayModeGame) {
            for (unsigned int i = 0; i < mTrackGraphs.size(); ++i) {
                ScoreTrackGraph *pGraph = mTrackGraphs[i];
                if (mLevel->GetTrack(i)->mKind == kTrackModeCatch) {
                    pGraph->mTrackData->AddPhrases(pGraph->GetPhraseDatabase());
                    pGraph->Slot12();
                }
                pGraph->GetPhraseDatabase()->Clear();
            }
        }
    }
    mGamer->CreateEnableMgr(&mTrackGraphs);
}

// 0x0018caa8
void GrooveWorld::CreateRenderer() {
    mRenderer = new Renderer;
    mDelayer->AddSink(mRenderer);
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->AddSink(mRenderer);

        TrackSelectMsg select;
        select.mUnknown04 = (*it)->Slot4();
        select.mUnknown08 = 0;
        select.mPosition = Mid::MBT(0);
        select.mUnknown10 = *it;
        mRenderer->Handle(&select);

        if ((*it)->Slot2() == kNoSeeker) {
            SeekerMsg seeker(*it);
            mRenderer->Handle(&seeker);
        }
    }
}

// 0x0018da60
void GrooveWorld::DestroyGraphs() {
    std::for_each(mTrackGraphs.begin(), mTrackGraphs.end(), ScoreTrackGraph::Delete);
    std::for_each(mUnknown44.begin(), mUnknown44.end(), BGTrackGraph::Delete);
    std::for_each(mUnknown50.begin(), mUnknown50.end(), BGTrackGraph::Delete);
    mTrackGraphs.clear();
    mUnknown44.clear();
    mUnknown50.clear();

    mInputMap->RemoveSink(mJoiner);
    delete mMuseSynth;
    mMuseSynth = nullptr;
    delete mTrackSelector;
    mTrackSelector = nullptr;
    delete mInputMap;
    mInputMap = nullptr;
    delete mJoiner;
    mJoiner = nullptr;
    delete mDelayer;
    mDelayer = nullptr;
    delete mGamer;
    mGamer = nullptr;
    delete mUnknown5c;
    mUnknown5c = nullptr;
    if (mUnknown1c != nullptr) {
        mUnknown1c->ClearSinks();
    }
}

// 0x0018e238
void GrooveWorld::StartSequencers() {
    std::for_each(
        mUnknown50.begin(), mUnknown50.end(), std::mem_fn(&BGTrackGraph::CallDeleteSequencer));
    std::for_each(
        mUnknown44.begin(), mUnknown44.end(), std::mem_fn(&BGTrackGraph::CallBuildSequencer));
    std::for_each(
        mTrackGraphs.begin(), mTrackGraphs.end(), std::mem_fn(&ScoreTrackGraph::CallSlot2));
    if (mUnknown5c != nullptr) {
        mUnknown5c->BuildSequencer();
    }
    mGamer->Start();
}

// 0x0018e6f0
void GrooveWorld::FinishSong() {
    if (QueryConfigFlag(kStreamedAudioQuery) != 0) {
        StopSoundBankMovie();
    }
    Application::shared()->GetSynth()->Slot13(kSynthSlot13Off);
    mGamer->Withdraw();

    if (Application::shared()->GetPlayMode() == kPlayModeJam) {
        IOBPreallocMemStream *pLog = Application::shared()->GetResetLog();
        int nSize = 0;
        char szTitle[kLogTextLength];
        memset(szTitle, 0, sizeof(szTitle));
        char szLevel[kLogTextLength];
        memset(szLevel, 0, sizeof(szLevel));
        {
            HxStr level = QueryConfigString(kLevelNameQuery);
            strcpy(szLevel, level.mStr != nullptr ? level.mStr : g_szEmptyString);
        }

        // The length word is written as a placeholder and patched once the phrases are in.
        const int nPlaceholder = nSize;
        const char bLeading = kLogMarker;
        pLog->Write(&nPlaceholder, sizeof(nPlaceholder)).WriteBytes(&bLeading, sizeof(bLeading));
        pLog->WriteBytes(szLevel, sizeof(szLevel));
        pLog->WriteBytes(szTitle, sizeof(szTitle));
        const char bTrailing = kLogMarker;
        pLog->WriteBytes(&bTrailing, sizeof(bTrailing));
        for (std::vector<ScoreTrackGraph *>::iterator it = mTrackGraphs.begin();
             it != mTrackGraphs.end();
             ++it) {
            (*it)->GetPhraseDatabase()->Save(*pLog);
        }
        nSize = pLog->Size();
        memcpy(pLog->Buffer(), &nSize, sizeof(nSize));
    }

    std::for_each(
        mTrackGraphs.begin(), mTrackGraphs.end(), std::mem_fn(&ScoreTrackGraph::CallSlot3));
    std::for_each(
        mUnknown44.begin(), mUnknown44.end(), std::mem_fn(&BGTrackGraph::CallDeleteSequencer));
    std::for_each(
        mUnknown50.begin(), mUnknown50.end(), std::mem_fn(&BGTrackGraph::CallDeleteSequencer));
    std::for_each(mPlayers.begin(), mPlayers.end(), std::mem_fn(&Player::CallSlot12));
    if (mUnknown5c != nullptr) {
        mUnknown5c->DeleteSequencer();
    }

    if (mUnknown94 == kExitMode2) {
        FuncCmd *pCommand = new FuncCmd(this, &GrooveWorld::EndLevel);
        mApp->GetWatchdogTimer()->PostIn(pCommand, Sch::Tick{kEndLevelDelayNs});
        Attachment::ReleaseIfSet(pCommand);
    } else {
        EndLevel();
    }
}

// 0x0018eb70
void GrooveWorld::EndLevel() {
    if (mUnknown94 == kExitMode3) {
        const Color black{0.0f, 0.0f, 0.0f, kOpaque};
        g_gfxDevice.SetClearColor(black);
    }
    mState = kStateEnded;

    EndGameMsg msg;
    msg.mRestart = mUnknown88;
    mApp->GetGameManager()->QueueMessage(&msg);

    if ((mUnknown94 == kExitMode1 || mUnknown94 == kExitMode2) &&
        (!mApp->IsJukeboxMode() || mUnknownb8 == 0)) {
        SetBankLoadProgressHook(MainLoop::KeepAliveDraw);
        Application::shared()->GetSynth()->LoadBankSet4();
    }
}

// 0x0018ec90
void GrooveWorld::StopLevel() {
    DisconnectPlayers();
    if (mDelayer != nullptr) {
        mDelayer->RemoveSink(mRenderer);
    }
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->RemoveSink(mRenderer);
    }
    delete mRenderer;
    mRenderer = nullptr;
    DestroyGraphs();
    mSongClock->Pause();
    mSongClock->SetSongTick(Mid::MBT(0));
    mState = kStateLoaded;
}

// 0x0018f140
void GrooveWorld::DisplayText(const HxStr &text) {
    if (mDelayer != nullptr) {
        TextMsg msg(text);
        mDelayer->Handle(&msg);
    }
}

// 0x001937a8
HxStr GrooveWorld::GetSongName() const {
    return mSongName;
}

// 0x00194de8
void GrooveWorld::AddNetPlayer(int nId,
                               [[maybe_unused]] int nUnused,
                               const HxStr &name,
                               const FreqAppearance *pAppearance) {
    (void)(name != ""); // Yes, the binary discards this comparison's result.
    Player *pPlayer = new NetPlayer(nId, nId, name, pAppearance);
    mPlayers.push_back(pPlayer);
}

// 0x0018c600
void GrooveWorld::AddLocalPlayer(int nId,
                                 int nInputSlot,
                                 [[maybe_unused]] int nUnused,
                                 const HxStr &colorName,
                                 MetPersonaData *pPersona) {
    (void)(colorName != ""); // Yes, the binary discards this comparison's result.
    int nTrack = nId;
    if (Application::shared()->GetGameMode() == kGameModeSolo) {
        nTrack = QueryConfigValue(kSoloTrackConfigCode) - 1;
    }
    Player *pPlayer =
        new LocalPlayer(nId, nInputSlot, colorName, &pPersona->mUnknown140, mSongClock, nTrack);
    mPlayers.push_back(pPlayer);
    mLocalPlayers.push_back(pPlayer);
}

// 0x00194ef0
void GrooveWorld::RemovePlayer(int nId) {
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        if ((*it)->mId20 == nId) {
            mPlayers.erase(it);
            return;
        }
    }
}

// 0x00194f88
void GrooveWorld::DestroyRenderer() {
    if (mDelayer != nullptr) {
        mDelayer->RemoveSink(GetRendererSink());
    }
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->RemoveSink(GetRendererSink());
    }
    delete mRenderer;
    mRenderer = nullptr;
}

// 0x00195058
void GrooveWorld::SavePhrases(OBStream &stream) {
    for (std::vector<ScoreTrackGraph *>::iterator it = mTrackGraphs.begin();
         it != mTrackGraphs.end();
         ++it) {
        (*it)->GetPhraseDatabase()->Save(stream);
    }
}

// 0x001950c0
void GrooveWorld::LoadPhrases(IBStream &stream, int bClearOwners) {
    for (std::vector<ScoreTrackGraph *>::iterator it = mTrackGraphs.begin();
         it != mTrackGraphs.end();
         ++it) {
        PhraseDatabase *pDatabase = (*it)->GetPhraseDatabase();
        pDatabase->Load(stream);
        if (bClearOwners != 0) {
            pDatabase->ClearOwners();
        }
    }
}

// 0x00195150
void GrooveWorld::EnableInput() {
    mInputMap->EnableEntries();
}

// 0x00195170
void GrooveWorld::PostExitMode1() {
    PostExit(kExitMode1, mUnknownb8, 0);
}

// 0x00195198
void GrooveWorld::PostExitMode2() {
    PostExit(kExitMode2, 0, 0);
}

// 0x001951c0
void GrooveWorld::PostExitMode3() {
    PostExit(kExitMode3, 0, 1);
}

// 0x001952a0
Sch::TickClock *GrooveWorld::GetSongClock() {
    return mSongClock;
}

// 0x001952a8
PlayMap *GrooveWorld::GetPlayMap() {
    return mLevel->OnUnknownSlot8();
}

// 0x001952d8
LevelData *GrooveWorld::GetLevel() {
    return mLevel;
}

// 0x001952e0
RendererBase *GrooveWorld::GetRendererSink() {
    return mRenderer;
}

// 0x00195378
void GrooveWorld::MarkStatsFlag() {
    mStats->mUnknown14 = 1;
}
