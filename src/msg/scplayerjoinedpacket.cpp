#include "msg/scplayerjoinedpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003ef7b0, PAL: 0x00427da0
SCPlayerJoinedPacket::SCPlayerJoinedPacket() {
}

// NTSC-U/C: 0x003e4cf8, PAL: 0x0041cf78
Message *SCPlayerJoinedPacket::New() {
    return new SCPlayerJoinedPacket;
}

// NTSC-U/C: 0x003ef718, PAL: 0x00427d08
// Clone allocates and hands off to the copy constructor at 0x003f31c8, which is
// the compiler expanding the implicit one.
Message *SCPlayerJoinedPacket::Clone() {
    return new SCPlayerJoinedPacket(*this);
}

// NTSC-U/C: 0x003ef790, PAL: 0x00427d80
int SCPlayerJoinedPacket::Type() {
    return g_nSCPlayerJoinedPacketType;
}

// NTSC-U/C: 0x003ef7a0, PAL: 0x00427d90
const char *SCPlayerJoinedPacket::Name() {
    return "SCPlayerJoinedPacket";
}

// NTSC-U/C: 0x003f2100, PAL: 0x0042a648
void SCPlayerJoinedPacket::Print(std::ostream &stream) {
    mPlayerInfo.Print(stream);
}

// NTSC-U/C: 0x003e5fb8, PAL: 0x0041e298
void SCPlayerJoinedPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    mPlayerInfo.Save(stream);
}

// NTSC-U/C: 0x003f2048, PAL: 0x0042a590
void SCPlayerJoinedPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    mPlayerInfo.Load(stream);
}
