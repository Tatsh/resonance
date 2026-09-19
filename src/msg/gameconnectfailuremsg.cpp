#include "msg/gameconnectfailuremsg.h"

// 0x003e1730
Message *GameConnectFailureMsg::Clone() {
    return new GameConnectFailureMsg(*this);
}

// 0x003e17d0
int GameConnectFailureMsg::Type() {
    return g_nGameConnectFailureMsgType;
}

// 0x003e17e0
const char *GameConnectFailureMsg::Name() {
    return "GameConnectFailureMsg";
}
