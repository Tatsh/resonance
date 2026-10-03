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

// NTSC-U/C: 0x00594968, PAL: 0x005d7d00
Sch::Playbacker::Playbacker(Scheduler *pWatchdog) : mWatchdog(pWatchdog) {
}

// NTSC-U/C: 0x00594988, PAL: 0x005d7d20
Sch::Playbacker::~Playbacker() {
    std::for_each(mCommands.begin(), mCommands.end(), Attachment::ReleaseIfSet);
    mCommands.erase(mCommands.begin(), mCommands.end());
}

// NTSC-U/C: 0x00594a78, PAL: 0x005d7e10
void Sch::Playbacker::Load(IBStream &stream) {
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

// NTSC-U/C: 0x005962e8, PAL: 0x005d96f0
void Sch::Playbacker::Start() {
    mWatchdog->mClock.Pause();
    mWatchdog->RestartClock();
    QueueRemaining();
    mWatchdog->mClock.Resume();
}

// NTSC-U/C: 0x00596330, PAL: 0x005d9738
void Sch::Playbacker::QueueRemaining() {
    while (mCursor != mCommands.end()) {
        mWatchdog->QueueReplayed(*mCursor);
        ++mCursor;
    }
}
