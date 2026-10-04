#include "msg/pausegamesystemmsg.h"

Message *PauseGameSystemMsg::New() {
    return new PauseGameSystemMsg;
}

Message *PauseGameSystemMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new PauseGameSystemMsg(*this);
}

int PauseGameSystemMsg::Type() {
    return g_nPauseGameSystemMsgType;
}

const char *PauseGameSystemMsg::GetName() const {
    return "PauseGameSystemMsg";
}
