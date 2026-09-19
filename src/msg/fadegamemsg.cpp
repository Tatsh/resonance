#include "msg/fadegamemsg.h"

// 0x003d7978
Message *FadeGameMsg::New() {
    return new FadeGameMsg;
}

// 0x00193f88. The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *FadeGameMsg::Clone() {
    return new FadeGameMsg(*this);
}

// 0x00193fd8
int FadeGameMsg::Type() {
    return g_nFadeGameMsgType;
}

// 0x00193fe8
const char *FadeGameMsg::Name() {
    return "FadeGameMsg";
}
