#include "msg/scstartplayingpacket.h"

// 0x003e4f40
Message *SCStartPlayingPacket::New() {
    return new SCStartPlayingPacket;
}

// 0x003f01f8. Clone allocates and hands off to the copy constructor at 0x003f3720, which is
// the compiler expanding the implicit one.
Message *SCStartPlayingPacket::Clone() {
    return new SCStartPlayingPacket(*this);
}

// 0x003f0270
int SCStartPlayingPacket::Type() {
    return g_nSCStartPlayingPacketType;
}

// 0x003f0280
const char *SCStartPlayingPacket::Name() {
    return "SCStartPlayingPacket";
}
