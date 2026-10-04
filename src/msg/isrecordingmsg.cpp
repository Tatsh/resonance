#include "msg/isrecordingmsg.h"

#include <iostream>

Message *IsRecordingMsg::New() {
    return new IsRecordingMsg;
}

Message *IsRecordingMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new IsRecordingMsg(*this);
}

int IsRecordingMsg::Type() {
    return g_nIsRecordingMsgType;
}

const char *IsRecordingMsg::GetName() const {
    return "IsRecordingMsg";
}

void IsRecordingMsg::PrintExtra(std::ostream &stream) const {
    stream << "IsRecordingMsg " << mIsRecording;
}
