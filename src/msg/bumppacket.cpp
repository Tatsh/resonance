#include "msg/bumppacket.h"

// 0x003f0e80. Clone allocates and hands off to the copy constructor at 0x003f3b58, which is
// the compiler expanding the implicit one.
Message *BumpPacket::Clone() {
    return new BumpPacket(*this);
}

// 0x003f0ef8
int BumpPacket::Type() {
    return g_nBumpPacketType;
}

// 0x003f0f08
const char *BumpPacket::Name() {
    return "BumpPacket";
}
