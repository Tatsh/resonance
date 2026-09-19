#include "msg/scloadlevelpacket.h"

// 0x003f13a0. Clone allocates and hands off to the copy constructor at 0x003f3c48, which is
// the compiler expanding the implicit one.
Message *SCLoadLevelPacket::Clone() {
    return new SCLoadLevelPacket(*this);
}

// 0x003f1418
int SCLoadLevelPacket::Type() {
    return g_nSCLoadLevelPacketType;
}

// 0x003f1428
const char *SCLoadLevelPacket::Name() {
    return "SCLoadLevelPacket";
}
