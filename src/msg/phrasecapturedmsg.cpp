#include "msg/phrasecapturedmsg.h"

// 0x003debe0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *PhraseCapturedMsg::Clone() {
    return new PhraseCapturedMsg(*this);
}

// 0x003dec68
int PhraseCapturedMsg::Type() {
    return g_nPhraseCapturedMsgType;
}

// 0x003dec78
const char *PhraseCapturedMsg::Name() {
    return "PhraseCapturedMsg";
}
