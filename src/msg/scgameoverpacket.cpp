#include "msg/scgameoverpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e50f0, PAL: 0x0041d388
Message *SCGameOverPacket::New() {
    return new SCGameOverPacket;
}

// NTSC-U/C: 0x003f15d0, PAL: 0x00429a98
// Clone allocates and hands off to the copy constructor at 0x003f3cd0, which is
// the compiler expanding the implicit one.
Message *SCGameOverPacket::Clone() {
    return new SCGameOverPacket(*this);
}

// NTSC-U/C: 0x003f1648, PAL: 0x00429b10
int SCGameOverPacket::Type() {
    return g_nSCGameOverPacketType;
}

// NTSC-U/C: 0x003f1658, PAL: 0x00429b20
const char *SCGameOverPacket::GetName() const {
    return "SCGameOverPacket";
}

// NTSC-U/C: 0x003f2a90, PAL: 0x0042afd8
void SCGameOverPacket::PrintExtra(std::ostream &stream) const {
    stream << mResult;
}

// NTSC-U/C: 0x003f2920, PAL: 0x0042ae68
void SCGameOverPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    stream << mResult;
}

// NTSC-U/C: 0x003f29e8, PAL: 0x0042af30
void SCGameOverPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    stream >> mResult;
}
