#include "msg/textmsg.h"

// 0x003d7118
Message *TextMsg::New() {
    return new TextMsg;
}

// 0x00193e10. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *TextMsg::Clone() {
    return new TextMsg(*this);
}

// 0x00193eb8
int TextMsg::Type() {
    return g_nTextMsgType;
}

// 0x00193ec8
const char *TextMsg::Name() {
    return "TextMsg";
}
