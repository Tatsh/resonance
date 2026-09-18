#include "msg/csclientstatuspacket.h"

// 0x003ef970. Clone allocates and hands off to the copy constructor at 0x003f3250, which is
// the compiler expanding the implicit one.
Message *CSClientStatusPacket::Clone() {
    return new CSClientStatusPacket(*this);
}

// 0x003ef9e8
int CSClientStatusPacket::Type() {
    return g_nCSClientStatusPacketType;
}

// 0x003ef9f8
const char *CSClientStatusPacket::Name() {
    return "CSClientStatusPacket";
}
