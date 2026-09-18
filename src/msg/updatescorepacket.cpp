#include "msg/updatescorepacket.h"

// 0x003f06c0. Clone allocates and hands off to the copy constructor at 0x003f3818, which is
// the compiler expanding the implicit one.
Message *UpdateScorePacket::Clone() {
    return new UpdateScorePacket(*this);
}

// 0x003f0738
int UpdateScorePacket::Type() {
    return g_nUpdateScorePacketType;
}

// 0x003f0748
const char *UpdateScorePacket::Name() {
    return "UpdateScorePacket";
}
