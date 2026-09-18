#include "msg/scgameoverpacket.h"

// 0x003f15d0. Clone allocates and hands off to the copy constructor at 0x003f3cd0, which is
// the compiler expanding the implicit one.
Message *SCGameOverPacket::Clone() {
    return new SCGameOverPacket(*this);
}

// 0x003f1648
int SCGameOverPacket::Type() {
    return g_nSCGameOverPacketType;
}

// 0x003f1658
const char *SCGameOverPacket::Name() {
    return "SCGameOverPacket";
}
