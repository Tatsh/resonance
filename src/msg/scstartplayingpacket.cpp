#include "msg/scstartplayingpacket.h"

Message *SCStartPlayingPacket::New() {
    return new SCStartPlayingPacket;
}

Message *SCStartPlayingPacket::Clone() {
    // The copy constructor at 0x003f3720 is the compiler expanding the implicit one.
    return new SCStartPlayingPacket(*this);
}

int SCStartPlayingPacket::Type() {
    return g_nSCStartPlayingPacketType;
}

const char *SCStartPlayingPacket::GetName() const {
    return "SCStartPlayingPacket";
}
