#include "msg/isrecordingmsg.h"

#include <iostream>

// 0x003d7d48
Message *IsRecordingMsg::New() {
    return new IsRecordingMsg;
}

// 0x003e2c20
// The field copies are the compiler expanding the implicit copy
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

// 0x003e44b0
void IsRecordingMsg::Print(std::ostream &stream) {
    stream << "IsRecordingMsg " << mIsRecording;
}
