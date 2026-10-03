#include "msg/scstartplayingpacket.h"

// NTSC-U/C: 0x003e4f40, PAL: 0x0041d1d8
Message *SCStartPlayingPacket::New() {
    return new SCStartPlayingPacket;
}

// NTSC-U/C: 0x003f01f8, PAL: 0x00428800
// Clone allocates and hands off to the copy constructor at 0x003f3720, which is
// the compiler expanding the implicit one.
Message *SCStartPlayingPacket::Clone() {
    return new SCStartPlayingPacket(*this);
}

// NTSC-U/C: 0x003f0270, PAL: 0x00428878
int SCStartPlayingPacket::Type() {
    return g_nSCStartPlayingPacketType;
}

// NTSC-U/C: 0x003f0280, PAL: 0x00428888
const char *SCStartPlayingPacket::Name() {
    return "SCStartPlayingPacket";
}
