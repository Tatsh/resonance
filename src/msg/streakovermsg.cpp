#include "msg/streakovermsg.h"

Message *StreakOverMsg::New() {
    return new StreakOverMsg;
}

Message *StreakOverMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new StreakOverMsg(*this);
}

int StreakOverMsg::Type() {
    return g_nStreakOverMsgType;
}

const char *StreakOverMsg::GetName() const {
    return "StreakOverMsg";
}
