#include "msg/scallclientsstatuspacket.h"

// 0x003efaa8. Clone allocates and hands off to the copy constructor at 0x003f3298, which is
// the compiler expanding the implicit one.
Message *SCAllClientsStatusPacket::Clone() {
    return new SCAllClientsStatusPacket(*this);
}

// 0x003efb20
int SCAllClientsStatusPacket::Type() {
    return g_nSCAllClientsStatusPacketType;
}

// 0x003efb30
const char *SCAllClientsStatusPacket::Name() {
    return "SCAllClientsStatusPacket";
}
