#include "msg/gamebeginmsg.h"

// 0x003d7888
Message *GameBeginMsg::New() {
    return new GameBeginMsg;
}

// 0x00193ad0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GameBeginMsg::Clone() {
    return new GameBeginMsg(*this);
}

// 0x00193b08
int GameBeginMsg::Type() {
    return g_nGameBeginMsgType;
}

// 0x00193b18
const char *GameBeginMsg::Name() {
    return "GameBeginMsg";
}
