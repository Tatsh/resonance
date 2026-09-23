#include "app/globals.h"

#include "app/mainloop.h"
#include "app/scriptsink.h"
#include "app/watchdog.h"
#include "app/watchdogtimer.h"
#include "game/gamemanagerimpl.h"
#include "stream/iobpreallocmemstream.h"
#include "synth/ps2hardsynth.h"

char g_abLogBuffer[kLogBufferSize];

// 0x00118c40
Globals::Globals() {
    mGameManager = nullptr;
    mMainLoop = nullptr;
    mWatchdog = nullptr;
    mWatchdogTimer = nullptr;
    mLog = nullptr;
    // Yes, the binary clears neither mSynth nor mScriptSink here.
}

// 0x00118c68
Globals::~Globals() {
}

// 0x001170d0
void Globals::Init() {
    mWatchdog = new Watchdog;
    mWatchdogTimer = new WatchdogTimer(mWatchdog);
    mWatchdogTimer->SetOrigin(0);
    mScriptSink = new ScriptSink(this);
    mGameManager = new GameManagerImpl;
    mMainLoop = new MainLoop(mWatchdog, mGameManager);
    // 0x004ee2f8. The log stream is the preallocated read-write stream rather than the
    // output interface: the constructor writes two base vtables and fills a 0x20-byte
    // object, and the output interface declares four virtuals and no member at all.
    mLog = new IOBPreallocMemStream(g_abLogBuffer, kLogBufferSize);
    CreateSynth();
}

// 0x00117270
void Globals::Shutdown() {
    if (mWatchdog != nullptr) {
        mWatchdog->Snapshot();
    }
    delete mSynth;
    mSynth = nullptr;
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

// 0x00118c98
void Globals::CreateSynth() {
    mSynth = CreatePs2HardSynth();
}

// 0x00118cc8
void Globals::RunMainLoop() {
    mMainLoop->Run();
}

// 0x00118eb8
GameManagerImpl *Globals::GetGameManager() {
    return mGameManager;
}

// 0x00118ec0
Ps2HardSynth *Globals::GetSynth() {
    return mSynth;
}

// 0x00118eb0
Watchdog *Globals::GetWatchdog() {
    return mWatchdog;
}

// 0x00118e70
WatchdogTimer *Globals::GetWatchdogTimer() {
    return mWatchdogTimer;
}

// 0x00118d40
GrooveWorld *Globals::GetWorld() {
    return mGameManager->GetWorld();
}

// 0x00118dd8
int Globals::GetPlayMode() {
    return GetGameManager()->GetPlayMode();
}

// 0x00118e08
int Globals::GetGameMode() {
    return GetGameManager()->GetGameMode();
}
