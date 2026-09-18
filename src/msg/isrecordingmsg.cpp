#include "msg/isrecordingmsg.h"

// 0x003e2c20. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *IsRecordingMsg::Clone() {
    return new IsRecordingMsg(*this);
}

// 0x003e2c68
int IsRecordingMsg::Type() {
    return g_nIsRecordingMsgType;
}

// 0x003e2c78
const char *IsRecordingMsg::Name() {
    return "IsRecordingMsg";
}
