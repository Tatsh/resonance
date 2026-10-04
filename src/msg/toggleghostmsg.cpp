#include "msg/toggleghostmsg.h"

Message *ToggleGhostMsg::New() {
    return new ToggleGhostMsg;
}

Message *ToggleGhostMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new ToggleGhostMsg(*this);
}

int ToggleGhostMsg::Type() {
    return g_nToggleGhostMsgType;
}

const char *ToggleGhostMsg::GetName() const {
    return "ToggleGhostMsg";
}
