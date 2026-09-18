#include "msg/juiceamountmsg.h"

// 0x003e08a8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *JuiceAmountMsg::Clone() {
    return new JuiceAmountMsg(*this);
}

// 0x003e08f8
int JuiceAmountMsg::Type() {
    return g_nJuiceAmountMsgType;
}

// 0x003e0908
const char *JuiceAmountMsg::Name() {
    return "JuiceAmountMsg";
}
