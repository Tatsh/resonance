#include "msg/spjoindenypacket.h"

// 0x003ef558. Clone allocates and hands off to the copy constructor at 0x003f3130, which is
// the compiler expanding the implicit one.
Message *SPJoinDenyPacket::Clone() {
    return new SPJoinDenyPacket(*this);
}

// 0x003ef5d0
int SPJoinDenyPacket::Type() {
    return g_nSPJoinDenyPacketType;
}

// 0x003ef5e0
const char *SPJoinDenyPacket::Name() {
    return "SPJoinDenyPacket";
}
