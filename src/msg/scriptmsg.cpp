#include "msg/scriptmsg.h"

// 0x003d7158
Message *ScriptMsg::New() {
    return new ScriptMsg;
}

// 0x0015a6c8. The string copy is the compiler expanding the implicit copy constructor.
Message *ScriptMsg::Clone() {
    return new ScriptMsg(*this);
}

// 0x0015a768
int ScriptMsg::Type() {
    return g_nScriptMsgType;
}

// 0x0015a778
const char *ScriptMsg::Name() {
    return "ScriptMsg";
}
