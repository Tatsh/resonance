#include "msg/metfreqendedmsg.h"

#include <iostream>

Message *MetFreqEndedMsg::New() {
    return new MetFreqEndedMsg;
}

Message *MetFreqEndedMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new MetFreqEndedMsg(*this);
}

int MetFreqEndedMsg::Type() {
    return g_nMetFreqEndedMsgType;
}

const char *MetFreqEndedMsg::GetName() const {
    return "MetFreqEndedMsg";
}

void MetFreqEndedMsg::PrintExtra(std::ostream &stream) const {
    stream << "MetFreqEndedMsg " << mStopJukebox;
}
