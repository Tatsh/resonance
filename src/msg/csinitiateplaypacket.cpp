#include "msg/csinitiateplaypacket.h"

// NTSC-U/C: 0x003e4ea0, PAL: 0x0041d138
Message *CSInitiatePlayPacket::New() {
    return new CSInitiatePlayPacket;
}

// NTSC-U/C: 0x003efca8, PAL: 0x004282b0
// Clone allocates and hands off to the copy constructor at 0x003f34a8, which is
// the compiler expanding the implicit one.
Message *CSInitiatePlayPacket::Clone() {
    return new CSInitiatePlayPacket(*this);
}

// NTSC-U/C: 0x003efd20, PAL: 0x00428328
int CSInitiatePlayPacket::Type() {
    return g_nCSInitiatePlayPacketType;
}

// NTSC-U/C: 0x003efd30, PAL: 0x00428338
const char *CSInitiatePlayPacket::GetName() const {
    return "CSInitiatePlayPacket";
}
