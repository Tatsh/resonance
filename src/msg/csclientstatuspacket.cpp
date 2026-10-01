#include "msg/csclientstatuspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e4e00
Message *CSClientStatusPacket::New() {
    return new CSClientStatusPacket;
}

// 0x003ef970
// Clone allocates and hands off to the copy constructor at 0x003f3250, which is
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
    stream << "ClientStatus: " << mStatus;
}

// 0x003e6090
void CSClientStatusPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int status = mStatus;
    stream.Write(&status, sizeof(status));

    // Yes, the binary moves the client identifier a second time.
    int clientIdAgain = mClientId;
    stream.Write(&clientIdAgain, sizeof(clientIdAgain));
}

// 0x003e6198
void CSClientStatusPacket::Load(IBStream &stream) {
    Packet::Load(stream);

    int status = 0;
    stream.Read(&status, sizeof(status));

    // Yes, the binary moves the client identifier a second time.
    stream.Read(&mClientId, sizeof(mClientId));

    mStatus = status;
}
