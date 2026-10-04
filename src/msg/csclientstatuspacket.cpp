#include "msg/csclientstatuspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *CSClientStatusPacket::New() {
    return new CSClientStatusPacket;
}

Message *CSClientStatusPacket::Clone() {
    // The copy constructor at 0x003f3250 is the compiler expanding the implicit one.
    return new CSClientStatusPacket(*this);
}

int CSClientStatusPacket::Type() {
    return g_nCSClientStatusPacketType;
}

const char *CSClientStatusPacket::GetName() const {
    return "CSClientStatusPacket";
}

void CSClientStatusPacket::PrintExtra(std::ostream &stream) const {
    stream << "ClientStatus: " << mStatus;
}

void CSClientStatusPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int status = mStatus;
    stream.WriteLE(&status, sizeof(status));

    // Yes, the binary moves the client identifier a second time.
    int clientIdAgain = mClientId;
    stream.WriteLE(&clientIdAgain, sizeof(clientIdAgain));
}

void CSClientStatusPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    int status = 0;
    stream.ReadLE(&status, sizeof(status));

    // Yes, the binary moves the client identifier a second time.
    stream.ReadLE(&mClientId, sizeof(mClientId));

    mStatus = status;
}
