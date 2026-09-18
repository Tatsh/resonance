#include "msg/stopriffmsg.h"

// 0x003da7f8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *StopRiffMsg::Clone() {
    return new StopRiffMsg(*this);
}

// 0x003da858
int StopRiffMsg::Type() {
    return g_nStopRiffMsgType;
}

// 0x003da868
const char *StopRiffMsg::Name() {
    return "StopRiffMsg";
}
