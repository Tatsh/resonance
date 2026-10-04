#include "msg/textmsg.h"

Message *TextMsg::New() {
    return new TextMsg;
}

Message *TextMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new TextMsg(*this);
}

int TextMsg::Type() {
    return g_nTextMsgType;
}

const char *TextMsg::GetName() const {
    return "TextMsg";
}
