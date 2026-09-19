#include "msg/cripplepacket.h"

// 0x003f0c60. Clone allocates and hands off to the copy constructor at 0x003f3938, which is
// the compiler expanding the implicit one.
Message *CripplePacket::Clone() {
    return new CripplePacket(*this);
}

// 0x003f0cd8
int CripplePacket::Type() {
    return g_nCripplePacketType;
}

// 0x003f0ce8
const char *CripplePacket::Name() {
    return "CripplePacket";
}
