#include "msg/multimusemsg.h"

// 0x003dc590. Clone allocates and hands off to the copy constructor at 0x003e38a8, which is
// the compiler expanding the implicit one.
Message *MultiMuseMsg::Clone() {
    return new MultiMuseMsg(*this);
}

// 0x003dc608
int MultiMuseMsg::Type() {
    return g_dwMultiMuseMsgType;
}

// 0x003dc618
const char *MultiMuseMsg::Name() {
    return "MultiMuseMsg";
}
