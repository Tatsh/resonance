#include "msg/gameovermsg.h"

// 0x003d7908
Message *GameOverMsg::New() {
    return new GameOverMsg;
}

// 0x00193bd8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GameOverMsg::Clone() {
    return new GameOverMsg(*this);
}

// 0x00193c10
int GameOverMsg::Type() {
    return g_nGameOverMsgType;
}

// 0x00193c20
const char *GameOverMsg::Name() {
    return "GameOverMsg";
}
