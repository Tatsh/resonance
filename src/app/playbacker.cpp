#include "app/playbacker.h"

#include <algorithm>

#include "app/attachment.h"
#include "app/scheduler.h"
#include "sch/cmdid.h"
#include "sch/command.h"
#include "sch/timedcommand.h"
#include "stream/ibstream.h"

namespace {

// Sch::Command::CmdID() of the EndRecordingCmd that ends a recording.
constexpr int kEndRecordingCmdId = 6;

} // namespace

Sch::Playbacker::Playbacker(Scheduler *pWatchdog) : mWatchdog(pWatchdog) {
}

Sch::Playbacker::~Playbacker() {
    std::for_each(mCommands.begin(), mCommands.end(), Attachment::ReleaseIfSet);
    mCommands.erase(mCommands.begin(), mCommands.end());
}

void Sch::Playbacker::Load(IBStream &stream) {
    mCommands.erase(mCommands.begin(), mCommands.end());
    for (;;) {
        Sch::TimedCommand *pCommand = new Sch::TimedCommand;
        pCommand->Load(stream);
        if (stream.Eof() != 0 || pCommand->mCommand->CmdID() == kEndRecordingCmdId) {
            delete pCommand;
            break;
        }
        CmdID::ReserveID(pCommand->mCmdID);
        mCommands.push_back(pCommand);
    }
    mCursor = mCommands.begin();
}

void Sch::Playbacker::Play() {
    mWatchdog->mClock.Pause();
    mWatchdog->ResetTimes();
    QueueRemaining();
    mWatchdog->mClock.Resume();
}

void Sch::Playbacker::QueueRemaining() {
    while (mCursor != mCommands.end()) {
        mWatchdog->QueueReplayed(*mCursor);
        ++mCursor;
    }
}
