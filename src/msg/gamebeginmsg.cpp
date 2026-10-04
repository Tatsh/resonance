#include "msg/gamebeginmsg.h"

Message *GameBeginMsg::New() {
    return new GameBeginMsg;
}

Message *GameBeginMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new GameBeginMsg(*this);
}

int GameBeginMsg::Type() {
    return g_nGameBeginMsgType;
}

const char *GameBeginMsg::GetName() const {
    return "GameBeginMsg";
}
