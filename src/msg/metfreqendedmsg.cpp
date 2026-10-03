#include "msg/metfreqendedmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7d80, PAL: 0x0040fc98
Message *MetFreqEndedMsg::New() {
    return new MetFreqEndedMsg;
}

// NTSC-U/C: 0x003e2db0, PAL: 0x0041b250
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MetFreqEndedMsg::Clone() {
    return new MetFreqEndedMsg(*this);
}

// NTSC-U/C: 0x003e2df8, PAL: 0x0041b298
int MetFreqEndedMsg::Type() {
    return g_nMetFreqEndedMsgType;
}

// NTSC-U/C: 0x003e2e08, PAL: 0x0041b2a8
const char *MetFreqEndedMsg::Name() {
    return "MetFreqEndedMsg";
}

// NTSC-U/C: 0x003e44f0, PAL: 0x0041c720
void MetFreqEndedMsg::Print(std::ostream &stream) {
    stream << "MetFreqEndedMsg " << mStopJukebox;
}
