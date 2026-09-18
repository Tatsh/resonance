#include "msg/buttonpowmsg.h"

// 0x003db220. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *ButtonPowMsg::Clone() {
    return new ButtonPowMsg(*this);
}

// 0x003db278
int ButtonPowMsg::Type() {
    return g_nButtonPowMsgType;
}

// 0x003db288
const char *ButtonPowMsg::Name() {
    return "ButtonPowMsg";
}
