#include "msg/lobbyconnectionlostmsg.h"

// 0x003e1c30
Message *LobbyConnectionLostMsg::Clone() {
    return new LobbyConnectionLostMsg(*this);
}

// 0x003e1cd0
int LobbyConnectionLostMsg::Type() {
    return g_nLobbyConnectionLostMsgType;
}

// 0x003e1ce0
const char *LobbyConnectionLostMsg::Name() {
    return "LobbyConnectionLostMsg";
}
