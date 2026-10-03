#include "msg/isrecordingmsg.h"

#include <iostream>

// NTSC-U/C: 0x003d7d48, PAL: 0x0040fc60
Message *IsRecordingMsg::New() {
    return new IsRecordingMsg;
}

// NTSC-U/C: 0x003e2c20, PAL: 0x0041b0c0
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *IsRecordingMsg::Clone() {
    return new IsRecordingMsg(*this);
}

// NTSC-U/C: 0x003e2c68, PAL: 0x0041b108
int IsRecordingMsg::Type() {
    return g_nIsRecordingMsgType;
}

// NTSC-U/C: 0x003e2c78, PAL: 0x0041b118
const char *IsRecordingMsg::Name() {
    return "IsRecordingMsg";
}

// NTSC-U/C: 0x003e44b0, PAL: 0x0041c6e0
void IsRecordingMsg::Print(std::ostream &stream) {
    stream << "IsRecordingMsg " << mIsRecording;
}
