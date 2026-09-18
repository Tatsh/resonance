#include "msg/trackselectpacket.h"

// 0x003f0848. Clone allocates and hands off to the copy constructor at 0x003f3868, which is
// the compiler expanding the implicit one.
Message *TrackSelectPacket::Clone() {
    return new TrackSelectPacket(*this);
}

// 0x003f08c0
int TrackSelectPacket::Type() {
    return g_nTrackSelectPacketType;
}

// 0x003f08d0
const char *TrackSelectPacket::Name() {
    return "TrackSelectPacket";
}
