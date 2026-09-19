#include "msg/spjoinacceptpacket.h"

// 0x003ef078. Clone allocates and hands off to the copy constructor at 0x003f2e48, which is
// the compiler expanding the implicit one.
Message *SPJoinAcceptPacket::Clone() {
    return new SPJoinAcceptPacket(*this);
}

// 0x003ef0f0
int SPJoinAcceptPacket::Type() {
    return g_nSPJoinAcceptPacketType;
}

// 0x003ef100
const char *SPJoinAcceptPacket::Name() {
    return "SPJoinAcceptPacket";
}
