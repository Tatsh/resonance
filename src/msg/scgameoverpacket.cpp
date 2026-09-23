#include "msg/scgameoverpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e50f0
Message *SCGameOverPacket::New() {
    return new SCGameOverPacket;
}

// 0x003f15d0
// Clone allocates and hands off to the copy constructor at 0x003f3cd0, which is
// the compiler expanding the implicit one.
Message *SCGameOverPacket::Clone() {
    return new SCGameOverPacket(*this);
}

// 0x003f1648
int SCGameOverPacket::Type() {
    return g_nSCGameOverPacketType;
}

// 0x003f1658
const char *SCGameOverPacket::Name() {
    return "SCGameOverPacket";
}

// 0x003f2a90
void SCGameOverPacket::Print(std::ostream &stream) {
    stream << mUnknown14;
}

// 0x003f2920
void SCGameOverPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    stream << mUnknown14;
}

// 0x003f29e8
void SCGameOverPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    stream >> mUnknown14;
}
