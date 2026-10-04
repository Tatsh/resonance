#include "msg/metstartnetlaunchmsg.h"

#include <iostream>

Message *MetStartNetLaunchMsg::New() {
    return new MetStartNetLaunchMsg;
}

Message *MetStartNetLaunchMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new MetStartNetLaunchMsg(*this);
}

int MetStartNetLaunchMsg::Type() {
    return g_nMetStartNetLaunchMsgType;
}

const char *MetStartNetLaunchMsg::GetName() const {
    return "MetStartNetLaunchMsg";
}

void MetStartNetLaunchMsg::PrintExtra(std::ostream &stream) const {
    stream << "MetStartNetLaunchMsg";
}
