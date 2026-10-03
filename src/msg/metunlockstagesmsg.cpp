#include "msg/metunlockstagesmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7db8, PAL: 0x0040fcd0
Message *MetUnlockStagesMsg::New() {
    return new MetUnlockStagesMsg;
}

// NTSC-U/C: 0x003e2f40, PAL: 0x0041b3e0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MetUnlockStagesMsg::Clone() {
    return new MetUnlockStagesMsg(*this);
}

// NTSC-U/C: 0x003e2f78, PAL: 0x0041b418
int MetUnlockStagesMsg::Type() {
    return g_nMetUnlockStagesMsgType;
}

// NTSC-U/C: 0x003e2f88, PAL: 0x0041b428
const char *MetUnlockStagesMsg::Name() {
    return "MetUnlockStagesMsg";
}

// NTSC-U/C: 0x003e4530, PAL: 0x0041c760
// Yes, the binary writes MetFreqEndedMsg's label here.
void MetUnlockStagesMsg::Print(std::ostream &stream) {
    stream << "MetFreqEndedMsg ";
}
