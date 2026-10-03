#include "msg/textmsg.h"

// NTSC-U/C: 0x003d7118, PAL: 0x0040f008
Message *TextMsg::New() {
    return new TextMsg;
}

// NTSC-U/C: 0x00193e10, PAL: 0x00199a48
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *TextMsg::Clone() {
    return new TextMsg(*this);
}

// NTSC-U/C: 0x00193eb8, PAL: 0x00199af0
int TextMsg::Type() {
    return g_nTextMsgType;
}

// NTSC-U/C: 0x00193ec8, PAL: 0x00199b00
const char *TextMsg::GetName() const {
    return "TextMsg";
}
