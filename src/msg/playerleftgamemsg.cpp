#include "msg/playerleftgamemsg.h"

// 0x003e1e88. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PlayerLeftGameMsg::Clone() {
    return new PlayerLeftGameMsg(*this);
}

// 0x003e1ed0
int PlayerLeftGameMsg::Type() {
    return g_nPlayerLeftGameMsgType;
}

// 0x003e1ee0
const char *PlayerLeftGameMsg::Name() {
    return "PlayerLeftGameMsg";
}
