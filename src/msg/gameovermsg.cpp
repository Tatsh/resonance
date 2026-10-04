#include "msg/gameovermsg.h"

Message *GameOverMsg::New() {
    return new GameOverMsg;
}

Message *GameOverMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new GameOverMsg(*this);
}

int GameOverMsg::Type() {
    return g_nGameOverMsgType;
}

const char *GameOverMsg::GetName() const {
    return "GameOverMsg";
}
