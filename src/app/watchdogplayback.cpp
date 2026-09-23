#include "app/watchdogplayback.h"

#include <algorithm>

#include "app/attachment.h"
#include "app/watchdog.h"
#include "sch/cmdid.h"
#include "sch/command.h"
#include "sch/timedcommand.h"
#include "stream/ibstream.h"

namespace {

// Sch::Command::CmdID() of the EndRecordingCmd that ends a recording.
constexpr int kEndRecordingCmdId = 6;

} // namespace

// 0x00594968
WatchdogPlayback::WatchdogPlayback(Watchdog *pWatchdog) : mWatchdog(pWatchdog) {
}

// 0x00594988
WatchdogPlayback::~WatchdogPlayback() {
    std::for_each(mCommands.begin(), mCommands.end(), Attachment::ReleaseIfSet);
    mCommands.erase(mCommands.begin(), mCommands.end());
}

// 0x00594a78
void WatchdogPlayback::Load(IBStream &stream) {
    mCommands.erase(mCommands.begin(), mCommands.end());
    for (;;) {
        Sch::TimedCommand *pCommand = new Sch::TimedCommand;
        pCommand->Load(stream);
        if (stream.Eof() != 0 || pCommand->mCommand->CmdID() == kEndRecordingCmdId) {
            delete pCommand;
            break;
        }
        CmdID::Reserve(pCommand->mCmdID);
        mCommands.push_back(pCommand);
    }
    mCursor = mCommands.begin();
}

// 0x005962e8
void WatchdogPlayback::Start() {
    mWatchdog->mClock.Pause();
    mWatchdog->RestartClock();
    QueueRemaining();
    mWatchdog->mClock.Resume();
}

// 0x00596330
void WatchdogPlayback::QueueRemaining() {
    while (mCursor != mCommands.end()) {
        mWatchdog->QueueReplayed(*mCursor);
        ++mCursor;
    }
}
