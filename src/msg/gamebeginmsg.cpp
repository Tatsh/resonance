#include "msg/gamebeginmsg.h"

// NTSC-U/C: 0x003d7888, PAL: 0x0040f788
Message *GameBeginMsg::New() {
    return new GameBeginMsg;
}

// NTSC-U/C: 0x00193ad0, PAL: 0x001996f8
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *GameBeginMsg::Clone() {
    return new GameBeginMsg(*this);
}

// NTSC-U/C: 0x00193b08, PAL: 0x00199730
int GameBeginMsg::Type() {
    return g_nGameBeginMsgType;
}

// NTSC-U/C: 0x00193b18, PAL: 0x00199740
const char *GameBeginMsg::GetName() const {
    return "GameBeginMsg";
}
