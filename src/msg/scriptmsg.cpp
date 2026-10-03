#include "msg/scriptmsg.h"

// NTSC-U/C: 0x003d7158, PAL: 0x0040f050
Message *ScriptMsg::New() {
    return new ScriptMsg;
}

// NTSC-U/C: 0x0015a6c8, PAL: 0x0015c448
// The string copy is the compiler expanding the implicit copy constructor.
Message *ScriptMsg::Clone() {
    return new ScriptMsg(*this);
}

// NTSC-U/C: 0x0015a768, PAL: 0x0015c4e8
int ScriptMsg::Type() {
    return g_nScriptMsgType;
}

// NTSC-U/C: 0x0015a778, PAL: 0x0015c4f8
const char *ScriptMsg::GetName() const {
    return "ScriptMsg";
}
