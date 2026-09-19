#include "msg/psjoinrequestpacket.h"

// 0x003eeeb8. Clone allocates and hands off to the copy constructor at 0x003f2dc0, which is
// the compiler expanding the implicit one.
Message *PSJoinRequestPacket::Clone() {
    return new PSJoinRequestPacket(*this);
}

// 0x003eef30
int PSJoinRequestPacket::Type() {
    return g_nPSJoinRequestPacketType;
}

// 0x003eef40
const char *PSJoinRequestPacket::Name() {
    return "PSJoinRequestPacket";
}
