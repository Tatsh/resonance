#include "msg/pitchriffmsg.h"

// 0x003da620. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PitchRiffMsg::Clone() {
    return new PitchRiffMsg(*this);
}

// 0x003da680
int PitchRiffMsg::Type() {
    return g_nPitchRiffMsgType;
}

// 0x003da690
const char *PitchRiffMsg::Name() {
    return "PitchRiffMsg";
}
