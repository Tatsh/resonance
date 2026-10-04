#include "msg/susgemmsg.h"

Message *SusGemMsg::New() {
    return new SusGemMsg;
}

Message *SusGemMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new SusGemMsg(*this);
}

int SusGemMsg::Type() {
    return g_nSusGemMsgType;
}

const char *SusGemMsg::GetName() const {
    return "SusGemMsg";
}
