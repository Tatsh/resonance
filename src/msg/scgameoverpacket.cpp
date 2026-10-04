#include "msg/scgameoverpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *SCGameOverPacket::New() {
    return new SCGameOverPacket;
}

Message *SCGameOverPacket::Clone() {
    // The copy constructor at 0x003f3cd0 is the compiler expanding the implicit one.
    return new SCGameOverPacket(*this);
}

int SCGameOverPacket::Type() {
    return g_nSCGameOverPacketType;
}

const char *SCGameOverPacket::GetName() const {
    return "SCGameOverPacket";
}

void SCGameOverPacket::PrintExtra(std::ostream &stream) const {
    stream << mResult;
}

void SCGameOverPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    stream << mResult;
}

void SCGameOverPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    stream >> mResult;
}
