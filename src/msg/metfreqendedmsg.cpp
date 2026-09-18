#include "msg/metfreqendedmsg.h"

// 0x003e2db0. The field copies are the compiler expanding the implicit copy
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
