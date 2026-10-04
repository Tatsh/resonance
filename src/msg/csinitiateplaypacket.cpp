#include "msg/csinitiateplaypacket.h"

Message *CSInitiatePlayPacket::New() {
    return new CSInitiatePlayPacket;
}

Message *CSInitiatePlayPacket::Clone() {
    // The copy constructor at 0x003f34a8 is the compiler expanding the implicit one.
    return new CSInitiatePlayPacket(*this);
}

int CSInitiatePlayPacket::Type() {
    return g_nCSInitiatePlayPacketType;
}

const char *CSInitiatePlayPacket::GetName() const {
    return "CSInitiatePlayPacket";
}
