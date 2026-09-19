#include "msg/scallplayersinfopacket.h"

// 0x003eff60. Clone allocates and hands off to the copy constructor at 0x003f34e8, which is
// the compiler expanding the implicit one.
Message *SCAllPlayersInfoPacket::Clone() {
    return new SCAllPlayersInfoPacket(*this);
}

// 0x003effd8
int SCAllPlayersInfoPacket::Type() {
    return g_nSCAllPlayersInfoPacketType;
}

// 0x003effe8
const char *SCAllPlayersInfoPacket::Name() {
    return "SCAllPlayersInfoPacket";
}
