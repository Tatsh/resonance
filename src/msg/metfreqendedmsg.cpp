#include "msg/metfreqendedmsg.h"

#include <iostream>

// 0x003d7d80
Message *MetFreqEndedMsg::New() {
    return new MetFreqEndedMsg;
}

// 0x003e2db0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *MetFreqEndedMsg::Clone() {
    return new MetFreqEndedMsg(*this);
}

// 0x003e2df8
int MetFreqEndedMsg::Type() {
    return g_nMetFreqEndedMsgType;
}

// 0x003e2e08
const char *MetFreqEndedMsg::Name() {
    return "MetFreqEndedMsg";
}

// 0x003e44f0
void MetFreqEndedMsg::Print(std::ostream &stream) {
    stream << "MetFreqEndedMsg " << mUnknownb8Clear;
}
