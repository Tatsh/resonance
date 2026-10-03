#include "app/recorder.h"

#include "sch/timedcommand.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x00596290, PAL: 0x005d9698
Sch::Recorder::Recorder(OBStream *pStream) : mStream(pStream) {
}

// NTSC-U/C: 0x005962a0, PAL: 0x005d96a8
void Sch::Recorder::Record(Sch::TimedCommand *pCommand) {
    pCommand->Save(*mStream);
    mStream->Reset();
}
