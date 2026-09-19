#include "msg/gameconnectionlostmsg.h"

// 0x003e19b0
Message *GameConnectionLostMsg::Clone() {
    return new GameConnectionLostMsg(*this);
}

// 0x003e1a50
int GameConnectionLostMsg::Type() {
    return g_nGameConnectionLostMsgType;
}

// 0x003e1a60
const char *GameConnectionLostMsg::Name() {
    return "GameConnectionLostMsg";
}
