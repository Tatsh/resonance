#include "msg/streakovermsg.h"

// 0x003dbcf8. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *StreakOverMsg::Clone() {
    return new StreakOverMsg(*this);
}

// 0x003dbd40
int StreakOverMsg::Type() {
    return g_nStreakOverMsgType;
}

// 0x003dbd50
const char *StreakOverMsg::Name() {
    return "StreakOverMsg";
}
