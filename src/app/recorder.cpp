#include "app/recorder.h"

#include "sch/timedcommand.h"
#include "stream/obstream.h"

Sch::Recorder::Recorder(OBStream *pStream) : mStream(pStream) {
}

void Sch::Recorder::Record(Sch::TimedCommand *pCommand) {
    pCommand->Save(*mStream);
    mStream->Reset();
}
