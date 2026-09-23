#include "msg/metunlockstagesmsg.h"

#include <iostream>

// 0x003d7db8
Message *MetUnlockStagesMsg::New() {
    return new MetUnlockStagesMsg;
}

// 0x003e2f40
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MetUnlockStagesMsg::Clone() {
    return new MetUnlockStagesMsg(*this);
}

// 0x003e2f78
int MetUnlockStagesMsg::Type() {
    return g_nMetUnlockStagesMsgType;
}

// 0x003e2f88
const char *MetUnlockStagesMsg::Name() {
    return "MetUnlockStagesMsg";
}

// 0x003e4530
// Yes, the binary writes MetFreqEndedMsg's label here.
void MetUnlockStagesMsg::Print(std::ostream &stream) {
    stream << "MetFreqEndedMsg ";
}
