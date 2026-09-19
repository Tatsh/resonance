#include "msg/bsloadlevelpacket.h"

// 0x003f1118. Clone allocates and hands off to the copy constructor at 0x003f3bc0, which is
// the compiler expanding the implicit one.
Message *BSLoadLevelPacket::Clone() {
    return new BSLoadLevelPacket(*this);
}

// 0x003f1190
int BSLoadLevelPacket::Type() {
    return g_nBSLoadLevelPacketType;
}

// 0x003f11a0
const char *BSLoadLevelPacket::Name() {
    return "BSLoadLevelPacket";
}
