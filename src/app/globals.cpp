#include "app/globals.h"

#include "app/mainloop.h"
#include "app/scheduler.h"
#include "app/scriptsink.h"
#include "app/timeclock.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/grooveworld.h"
#include "mid/tick.h"
#include "sch/tempomap.h"
#include "sch/tickclock.h"
#include "stream/iobpreallocmemstream.h"
#include "synth/ps2hardsynth.h"

// NTSC-U/C: 0x0086f7d0, PAL: 0x008b3ed0
char g_abLogBuffer[kLogBufferSize];

Globals::Globals()
    : mGameManager(nullptr), mMainLoop(nullptr), mWatchdog(nullptr), mWatchdogTimer(nullptr),
      mLog(nullptr) {
    // Yes, the binary clears neither mSynth nor mScriptSink here.
}

Globals::~Globals() {
}

void Globals::Init() {
    mWatchdog = new Sch::Scheduler;
    mWatchdogTimer = new Sch::TimeClock(mWatchdog);
    mWatchdogTimer->SetOrigin(0);
    mScriptSink = new ScriptSink(this);
    mGameManager = new GameManagerImpl;
    mMainLoop = new MainLoop(1, mWatchdog, mGameManager);
    // NTSC-U/C: 0x004ee2f8, PAL: 0x0052cea0
    // The log stream is the preallocated read-write stream rather than the
    // output interface: the constructor writes two base vtables and fills a 0x20-byte
    // object, and the output interface declares four virtuals and no member at all.
    mLog = new IOBPreallocMemStream(g_abLogBuffer, kLogBufferSize);
    CreateSynth();
}

void Globals::Shutdown() {
    if (mWatchdog != nullptr) {
        mWatchdog->Snapshot();
    }
#ifndef VIDEO_STANDARD_PAL
    delete mSynth;
    mSynth = nullptr;
#endif
    delete mMainLoop;
    mMainLoop = nullptr;
    delete mGameManager;
    mGameManager = nullptr;
    delete mScriptSink;
    mScriptSink = nullptr;
    delete mWatchdogTimer;
    mWatchdogTimer = nullptr;
    delete mWatchdog;
    mWatchdog = nullptr;
    delete mLog;
    mLog = nullptr;
}

void Globals::CreateSynth() {
    mSynth = CreatePs2HardSynth();
}

void Globals::RunMainLoop() {
    mMainLoop->Run();
}

#ifdef VIDEO_STANDARD_PAL
void Globals::StopMainLoop() {
    mMainLoop->Stop();
}
#endif

GameManagerImpl *Globals::GetGameManager() {
    return mGameManager;
}

Ps2HardSynth *Globals::GetSynth() {
    return mSynth;
}

Sch::Scheduler *Globals::GetWatchdog() {
    return mWatchdog;
}

Sch::TimeClock *Globals::GetWatchdogTimer() {
    return mWatchdogTimer;
}

ScriptSink *Globals::GetScriptSink() {
    return mScriptSink;
}

IOBPreallocMemStream *Globals::GetLog() {
    return mLog;
}

IOBPreallocMemStream *Globals::GetResetLog() {
    mLog->Reset();
    return mLog;
}

GrooveWorld *Globals::GetWorld() {
    return mGameManager->GetWorld();
}

MetaGameWorld *Globals::GetMetaWorld() {
    return mGameManager->GetMetaWorld();
}

int Globals::GetUnwrittenValue() {
    return mGameManager->GetUnwrittenValue();
}

Sch::TickClock *Globals::GetSongClock() {
    return GetWorld()->GetSongClock();
}

PlayMap *Globals::GetPlayMap() {
    return GetWorld()->GetPlayMap();
}

int Globals::IsJukeboxMode() {
    return GetGameManager()->GetParams()->mJukeboxMode;
}

LevelData *Globals::GetLevel() {
    return GetWorld()->GetLevel();
}

int Globals::GetTempo() {
    Sch::TempoMap *pTempoMap = GetSongClock()->mTempoMap;
    (void)Sch::Tick::IsInRange(0); // Yes, the binary discards this call's result.
    return pTempoMap->mMicrosecondsPerQuarter;
}

int Globals::GetPlayMode() {
    return GetGameManager()->GetPlayMode();
}

int Globals::GetGameMode() {
    return GetGameManager()->GetGameMode();
}
