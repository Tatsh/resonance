#include "msg/nowbarmsg.h"

// 0x003d7680
Message *NowBarMsg::New() {
    return new NowBarMsg;
}

// 0x0019fa20
// The field copies are the compiler expanding the implicit copy
// constructor, so the allocation tag is the only part written here.
Message *NowBarMsg::Clone() {
    return new NowBarMsg(*this);
}

// 0x0019fa78
int NowBarMsg::Type() {
    return g_nNowBarMsgType;
}

// 0x0019fa88
const char *NowBarMsg::Name() {
    return "NowBarMsg";
}
