#include "msg/csclientstatuspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003ef970. Clone allocates and hands off to the copy constructor at 0x003f3250, which is
// the compiler expanding the implicit one.
Message *CSClientStatusPacket::Clone() {
    return new CSClientStatusPacket(*this);
}

// 0x003ef9e8
int CSClientStatusPacket::Type() {
    return g_nCSClientStatusPacketType;
}

// 0x003ef9f8
const char *CSClientStatusPacket::Name() {
    return "CSClientStatusPacket";
}

// 0x003f2120
void CSClientStatusPacket::Print(std::ostream &stream) {
    stream << "ClientStatus: " << mUnknown14;
}

// 0x003e6090
void CSClientStatusPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int unknown14 = mUnknown14;
    stream.Write(&unknown14, sizeof(unknown14));

    // Yes, the binary moves the base word at +0x0c a second time.
    int unknown0cAgain = mUnknown0c;
    stream.Write(&unknown0cAgain, sizeof(unknown0cAgain));
}

// 0x003e6198
void CSClientStatusPacket::Load(IBStream &stream) {
    Packet::Load(stream);

    int unknown14 = 0;
    stream.Read(&unknown14, sizeof(unknown14));

    // Yes, the binary moves the base word at +0x0c a second time.
    stream.Read(&mUnknown0c, sizeof(mUnknown0c));

    mUnknown14 = unknown14;
}
