#include "msg/unpausegamesystemmsg.h"

Message *UnpauseGameSystemMsg::New() {
    return new UnpauseGameSystemMsg;
}

Message *UnpauseGameSystemMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new UnpauseGameSystemMsg(*this);
}

int UnpauseGameSystemMsg::Type() {
    return g_nUnpauseGameSystemMsgType;
}

const char *UnpauseGameSystemMsg::GetName() const {
    return "UnpauseGameSystemMsg";
}
