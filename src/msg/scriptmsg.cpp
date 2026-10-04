#include "msg/scriptmsg.h"

Message *ScriptMsg::New() {
    return new ScriptMsg;
}

Message *ScriptMsg::Clone() {
    // The string copy is the compiler expanding the implicit copy constructor.
    return new ScriptMsg(*this);
}

int ScriptMsg::Type() {
    return g_nScriptMsgType;
}

const char *ScriptMsg::GetName() const {
    return "ScriptMsg";
}
