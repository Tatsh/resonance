#include "msg/scplayerjoinedpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

SCPlayerJoinedPacket::SCPlayerJoinedPacket() {
}

Message *SCPlayerJoinedPacket::New() {
    return new SCPlayerJoinedPacket;
}

Message *SCPlayerJoinedPacket::Clone() {
    // The copy constructor at 0x003f31c8 is the compiler expanding the implicit one.
    return new SCPlayerJoinedPacket(*this);
}

int SCPlayerJoinedPacket::Type() {
    return g_nSCPlayerJoinedPacketType;
}

const char *SCPlayerJoinedPacket::GetName() const {
    return "SCPlayerJoinedPacket";
}

void SCPlayerJoinedPacket::PrintExtra(std::ostream &stream) const {
    mPlayerInfo.Print(stream);
}

void SCPlayerJoinedPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mPlayerInfo.Save(stream);
}

void SCPlayerJoinedPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mPlayerInfo.Load(stream);
}
