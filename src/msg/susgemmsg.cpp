#include "msg/susgemmsg.h"

// 0x003d7580
Message *SusGemMsg::New() {
    return new SusGemMsg;
}

// 0x001a44d0. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *SusGemMsg::Clone() {
    return new SusGemMsg(*this);
}

// 0x001a4540
int SusGemMsg::Type() {
    return g_nSusGemMsgType;
}

// 0x001a4550
const char *SusGemMsg::Name() {
    return "SusGemMsg";
}
