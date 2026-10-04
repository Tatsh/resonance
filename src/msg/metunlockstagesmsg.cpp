#include "msg/metunlockstagesmsg.h"

#include <iostream>

Message *MetUnlockStagesMsg::New() {
    return new MetUnlockStagesMsg;
}

Message *MetUnlockStagesMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new MetUnlockStagesMsg(*this);
}

int MetUnlockStagesMsg::Type() {
    return g_nMetUnlockStagesMsgType;
}

const char *MetUnlockStagesMsg::GetName() const {
    return "MetUnlockStagesMsg";
}

void MetUnlockStagesMsg::PrintExtra(std::ostream &stream) const {
    stream << "MetFreqEndedMsg "; // Yes, the binary writes MetFreqEndedMsg's label here.
}
