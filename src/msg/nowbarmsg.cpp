#include "msg/nowbarmsg.h"

Message *NowBarMsg::New() {
    return new NowBarMsg;
}

Message *NowBarMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor.
    // The allocation tag is the only part written here.
    return new NowBarMsg(*this);
}

int NowBarMsg::Type() {
    return g_nNowBarMsgType;
}

const char *NowBarMsg::GetName() const {
    return "NowBarMsg";
}
