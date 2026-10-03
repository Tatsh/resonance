#include "msg/gameovermsg.h"

// NTSC-U/C: 0x003d7908, PAL: 0x0040f808
Message *GameOverMsg::New() {
    return new GameOverMsg;
}

// NTSC-U/C: 0x00193bd8, PAL: 0x00199800
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GameOverMsg::Clone() {
    return new GameOverMsg(*this);
}

// NTSC-U/C: 0x00193c10, PAL: 0x00199838
int GameOverMsg::Type() {
    return g_nGameOverMsgType;
}

// NTSC-U/C: 0x00193c20, PAL: 0x00199848
const char *GameOverMsg::GetName() const {
    return "GameOverMsg";
}
