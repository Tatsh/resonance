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

// NTSC-U/C: 0x00118c40, PAL: 0x00119190
Globals::Globals() {
    mGameManager = nullptr;
    mMainLoop = nullptr;
    mWatchdog = nullptr;
    mWatchdogTimer = nullptr;
    mLog = nullptr;
    // Yes, the binary clears neither mSynth nor mScriptSink here.
}

// NTSC-U/C: 0x00118c68, PAL: 0x001191b8
Globals::~Globals() {
}

// NTSC-U/C: 0x001170d0, PAL: 0x001175a0
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

// NTSC-U/C: 0x00117270, PAL: 0x00117740
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

// NTSC-U/C: 0x00118c98, PAL: 0x001191e8
void Globals::CreateSynth() {
    mSynth = CreatePs2HardSynth();
}

// NTSC-U/C: 0x00118cc8, PAL: 0x00119218
void Globals::RunMainLoop() {
    mMainLoop->Run();
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x00119238
void Globals::StopMainLoop() {
    mMainLoop->Stop();
}
#endif

// NTSC-U/C: 0x00118eb8, PAL: 0x00119418
GameManagerImpl *Globals::GetGameManager() {
    return mGameManager;
}

// NTSC-U/C: 0x00118ec0, PAL: 0x00119420
Ps2HardSynth *Globals::GetSynth() {
    return mSynth;
}

// NTSC-U/C: 0x00118eb0, PAL: 0x00119410
Sch::Scheduler *Globals::GetWatchdog() {
    return mWatchdog;
}

// NTSC-U/C: 0x00118e70, PAL: 0x001193d0
Sch::TimeClock *Globals::GetWatchdogTimer() {
    return mWatchdogTimer;
}

// NTSC-U/C: 0x00118ef8, PAL: 0x00119458
ScriptSink *Globals::GetScriptSink() {
    return mScriptSink;
}

// NTSC-U/C: 0x00118f00, PAL: 0x00119460
IOBPreallocMemStream *Globals::GetLog() {
    return mLog;
}

// NTSC-U/C: 0x00118f08, PAL: 0x00119468
IOBPreallocMemStream *Globals::GetResetLog() {
    mLog->Reset();
    return mLog;
}

// NTSC-U/C: 0x00118d40, PAL: 0x001192a0
GrooveWorld *Globals::GetWorld() {
    return mGameManager->GetWorld();
}

// NTSC-U/C: 0x00118d70, PAL: 0x001192d0
MetaGameWorld *Globals::GetMetaWorld() {
    return mGameManager->GetMetaWorld();
}

// NTSC-U/C: 0x00118ec8, PAL: 0x00119428
int Globals::GetUnwrittenValue() {
    return mGameManager->GetUnwrittenValue();
}

// NTSC-U/C: 0x00118e78, PAL: 0x001193d8
Sch::TickClock *Globals::GetSongClock() {
    return GetWorld()->GetSongClock();
}

// NTSC-U/C: 0x00118da0, PAL: 0x00119300
PlayMap *Globals::GetPlayMap() {
    return GetWorld()->GetPlayMap();
}

// NTSC-U/C: 0x00118e38, PAL: 0x00119398
int Globals::IsJukeboxMode() {
    return GetGameManager()->GetParams()->mJukeboxMode;
}

// NTSC-U/C: 0x00118d18, PAL: 0x00119278
LevelData *Globals::GetLevel() {
    return GetWorld()->GetLevel();
}

// NTSC-U/C: 0x00118ce8, PAL: 0x00119248
int Globals::GetTempo() {
    Sch::TempoMap *pTempoMap = GetSongClock()->mTempoMap;
    (void)Sch::Tick::IsInRange(0); // Yes, the binary discards this call's result.
    return pTempoMap->mMicrosecondsPerQuarter;
}

// NTSC-U/C: 0x00118dd8, PAL: 0x00119338
int Globals::GetPlayMode() {
    return GetGameManager()->GetPlayMode();
}

// NTSC-U/C: 0x00118e08, PAL: 0x00119368
int Globals::GetGameMode() {
    return GetGameManager()->GetGameMode();
}
