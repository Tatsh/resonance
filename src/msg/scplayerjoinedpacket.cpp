#include "msg/scplayerjoinedpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003ef7b0
SCPlayerJoinedPacket::SCPlayerJoinedPacket() {
}

// 0x003e4cf8
Message *SCPlayerJoinedPacket::New() {
    return new SCPlayerJoinedPacket;
}

// 0x003ef718
// Clone allocates and hands off to the copy constructor at 0x003f31c8, which is
// the compiler expanding the implicit one.
Message *SCPlayerJoinedPacket::Clone() {
    return new SCPlayerJoinedPacket(*this);
}

// 0x003ef790
int SCPlayerJoinedPacket::Type() {
    return g_nSCPlayerJoinedPacketType;
}

// 0x003ef7a0
const char *SCPlayerJoinedPacket::Name() {
    return "SCPlayerJoinedPacket";
}

// 0x003f2100
void SCPlayerJoinedPacket::Print(std::ostream &stream) {
    mUnknown14.Print(stream);
}

// 0x003e5fb8
void SCPlayerJoinedPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    mUnknown14.Save(stream);
}

// 0x003f2048
void SCPlayerJoinedPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    mUnknown14.Load(stream);
}
