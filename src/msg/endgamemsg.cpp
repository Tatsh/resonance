#include "msg/endgamemsg.h"

// 0x003d7b88
Message *EndGameMsg::New() {
    return new EndGameMsg;
}

// 0x001939b8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *EndGameMsg::Clone() {
    return new EndGameMsg(*this);
}

// 0x00193a00
int EndGameMsg::Type() {
    return g_nEndGameMsgType;
}

// 0x00193a10
const char *EndGameMsg::Name() {
    return "EndGameMsg";
}
