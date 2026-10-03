#include "msg/csclientstatuspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e4e00, PAL: 0x0041d098
Message *CSClientStatusPacket::New() {
    return new CSClientStatusPacket;
}

// NTSC-U/C: 0x003ef970, PAL: 0x00427f78
// Clone allocates and hands off to the copy constructor at 0x003f3250, which is
// the compiler expanding the implicit one.
Message *CSClientStatusPacket::Clone() {
    return new CSClientStatusPacket(*this);
}

// NTSC-U/C: 0x003ef9e8, PAL: 0x00427ff0
int CSClientStatusPacket::Type() {
    return g_nCSClientStatusPacketType;
}

// NTSC-U/C: 0x003ef9f8, PAL: 0x00428000
const char *CSClientStatusPacket::GetName() const {
    return "CSClientStatusPacket";
}

// NTSC-U/C: 0x003f2120, PAL: 0x0042a668
void CSClientStatusPacket::PrintExtra(std::ostream &stream) const {
    stream << "ClientStatus: " << mStatus;
}

// NTSC-U/C: 0x003e6090, PAL: 0x0041e370
void CSClientStatusPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int status = mStatus;
    stream.Write(&status, sizeof(status));

    // Yes, the binary moves the client identifier a second time.
    int clientIdAgain = mClientId;
    stream.Write(&clientIdAgain, sizeof(clientIdAgain));
}

// NTSC-U/C: 0x003e6198, PAL: 0x0041e478
void CSClientStatusPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    int status = 0;
    stream.Read(&status, sizeof(status));

    // Yes, the binary moves the client identifier a second time.
    stream.Read(&mClientId, sizeof(mClientId));

    mStatus = status;
}
