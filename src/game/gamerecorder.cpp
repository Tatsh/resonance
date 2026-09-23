#include "game/gamerecorder.h"

#include "app/application.h"
#include "app/watchdog.h"
#include "app/watchdogtimer.h"
#include "game/endrecordingcmd.h"
#include "sch/cmdid.h"
#include "sch/tick.h"
#include "stream/obfilestream.h"

namespace {

// The handle a post starts with, before the scheduler allocates one.
constexpr int kUnallocatedCommand = -2;

// The end of a recording is itself recorded.
constexpr int kRecordable = 1;

} // namespace

// 0x0010f010
GameRecorder::GameRecorder(GameManagerImpl *pManager) : mManager(pManager), mStream(nullptr) {
}

// 0x0010f020
GameRecorder::~GameRecorder() {
    delete mStream;
}

// 0x0010cea0
void GameRecorder::ScheduleEnd() {
    EndRecordingCmd *pCommand = new EndRecordingCmd(this);
    CmdID id;
    id.mValue = kUnallocatedCommand;
    const Sch::Tick now{0};
    Application::shared()->GetWatchdogTimer()->PostIn(pCommand, now, id, kRecordable);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x0010f080
void GameRecorder::EndRecording() {
    Application::shared()->GetWatchdog()->Close();
    delete mStream;
    mStream = nullptr;
}
