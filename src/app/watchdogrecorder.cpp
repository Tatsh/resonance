#include "app/watchdogrecorder.h"

#include "sch/timedcommand.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x00596290, PAL: 0x005d9698
WatchdogRecorder::WatchdogRecorder(OBStream *pStream) : mStream(pStream) {
}

// NTSC-U/C: 0x005962a0, PAL: 0x005d96a8
void WatchdogRecorder::Record(Sch::TimedCommand *pCommand) {
    pCommand->Save(*mStream);
    mStream->Reset();
}
