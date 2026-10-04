#include "msg/fadegamemsg.h"

Message *FadeGameMsg::New() {
    return new FadeGameMsg;
}

Message *FadeGameMsg::Clone() {
    // The field copies are the compiler expanding the implicit copy constructor. The allocation
    // tag is the only part written here.
    return new FadeGameMsg(*this);
}

int FadeGameMsg::Type() {
    return g_nFadeGameMsgType;
}

const char *FadeGameMsg::GetName() const {
    return "FadeGameMsg";
}
