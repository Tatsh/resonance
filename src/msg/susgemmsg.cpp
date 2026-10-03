#include "msg/susgemmsg.h"

// NTSC-U/C: 0x003d7580, PAL: 0x0040f480
Message *SusGemMsg::New() {
    return new SusGemMsg;
}

// NTSC-U/C: 0x001a44d0, PAL: 0x001aa238
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SusGemMsg::Clone() {
    return new SusGemMsg(*this);
}

// NTSC-U/C: 0x001a4540, PAL: 0x001aa2a8
int SusGemMsg::Type() {
    return g_nSusGemMsgType;
}

// NTSC-U/C: 0x001a4550, PAL: 0x001aa2b8
const char *SusGemMsg::GetName() const {
    return "SusGemMsg";
}
