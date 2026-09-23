#include "app/watchdogrecorder.h"

#include "sch/timedcommand.h"
#include "stream/obstream.h"

// 0x00596290
WatchdogRecorder::WatchdogRecorder(OBStream *pStream) : mStream(pStream) {
}

// 0x005962a0
void WatchdogRecorder::Record(Sch::TimedCommand *pCommand) {
    pCommand->Save(*mStream);
    mStream->Reset();
}
