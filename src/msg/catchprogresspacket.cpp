#include "msg/catchprogresspacket.h"

// 0x003f0a70. Clone allocates and hands off to the copy constructor at 0x003f38d0, which is
// the compiler expanding the implicit one.
Message *CatchProgressPacket::Clone() {
    return new CatchProgressPacket(*this);
}

// 0x003f0ae8
int CatchProgressPacket::Type() {
    return g_nCatchProgressPacketType;
}

// 0x003f0af8
const char *CatchProgressPacket::Name() {
    return "CatchProgressPacket";
}
