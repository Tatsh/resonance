#include "msg/phrasemuffedmsg.h"

// 0x003e0038. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PhraseMuffedMsg::Clone() {
    return new PhraseMuffedMsg(*this);
}

// 0x003e0098
int PhraseMuffedMsg::Type() {
    return g_nPhraseMuffedMsgType;
}

// 0x003e00a8
const char *PhraseMuffedMsg::Name() {
    return "PhraseMuffedMsg";
}
