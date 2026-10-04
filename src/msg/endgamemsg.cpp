#include "msg/endgamemsg.h"

Message *EndGameMsg::New() {
    return new EndGameMsg;
}

Message *EndGameMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new EndGameMsg(*this);
}

int EndGameMsg::Type() {
    return g_nEndGameMsgType;
}

const char *EndGameMsg::GetName() const {
    return "EndGameMsg";
}
