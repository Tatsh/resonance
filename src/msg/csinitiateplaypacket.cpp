#include "msg/csinitiateplaypacket.h"

// 0x003efca8. Clone allocates and hands off to the copy constructor at 0x003f34a8, which is
// the compiler expanding the implicit one.
Message *CSInitiatePlayPacket::Clone() {
    return new CSInitiatePlayPacket(*this);
}

// 0x003efd20
int CSInitiatePlayPacket::Type() {
    return g_nCSInitiatePlayPacketType;
}

// 0x003efd30
const char *CSInitiatePlayPacket::Name() {
    return "CSInitiatePlayPacket";
}
