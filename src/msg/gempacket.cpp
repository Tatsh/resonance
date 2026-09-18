#include "msg/gempacket.h"

// 0x003f1750. Clone allocates and hands off to the copy constructor at 0x003f3d18, which is
// the compiler expanding the implicit one.
Message *GemPacket::Clone() {
    return new GemPacket(*this);
}

// 0x003f17c8
int GemPacket::Type() {
    return g_nGemPacketType;
}

// 0x003f17d8
const char *GemPacket::Name() {
    return "GemPacket";
}
