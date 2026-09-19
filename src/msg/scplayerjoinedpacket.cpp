#include "msg/scplayerjoinedpacket.h"

// 0x003ef718. Clone allocates and hands off to the copy constructor at 0x003f31c8, which is
// the compiler expanding the implicit one.
Message *SCPlayerJoinedPacket::Clone() {
    return new SCPlayerJoinedPacket(*this);
}

// 0x003ef790
int SCPlayerJoinedPacket::Type() {
    return g_nSCPlayerJoinedPacketType;
}

// 0x003ef7a0
const char *SCPlayerJoinedPacket::Name() {
    return "SCPlayerJoinedPacket";
}
