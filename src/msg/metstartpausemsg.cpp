#include "msg/metstartpausemsg.h"

#include <iostream>

Message *MetStartPauseMsg::New() {
    return new MetStartPauseMsg;
}

Message *MetStartPauseMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new MetStartPauseMsg(*this);
}

int MetStartPauseMsg::Type() {
    return g_nMetStartPauseMsgType;
}

const char *MetStartPauseMsg::GetName() const {
    return "MetStartPauseMsg";
}

void MetStartPauseMsg::PrintExtra(std::ostream &stream) const {
    stream << "MetStartPauseMsg";
}
