#include "msg/gameconnectsuccessmsg.h"

#include <iostream>

Message *GameConnectSuccessMsg::New() {
    return new GameConnectSuccessMsg;
}

Message *GameConnectSuccessMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new GameConnectSuccessMsg(*this);
}

int GameConnectSuccessMsg::Type() {
    return g_nGameConnectSuccessMsgType;
}

const char *GameConnectSuccessMsg::GetName() const {
    return "GameConnectSuccessMsg";
}

void GameConnectSuccessMsg::PrintExtra(std::ostream &) const {
}
