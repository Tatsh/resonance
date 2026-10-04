#include "msg/showeraseeffectmsg.h"

Message *ShowEraseEffectMsg::New() {
    return new ShowEraseEffectMsg;
}

Message *ShowEraseEffectMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new ShowEraseEffectMsg(*this);
}

int ShowEraseEffectMsg::Type() {
    return g_nShowEraseEffectMsgType;
}

const char *ShowEraseEffectMsg::GetName() const {
    return "ShowEraseEffectMsg";
}
