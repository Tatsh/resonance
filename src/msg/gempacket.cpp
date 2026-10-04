#include "msg/gempacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *GemPacket::New() {
    return new GemPacket;
}

Message *GemPacket::Clone() {
    // The copy constructor at 0x003f3d18 is the compiler expanding the implicit one.
    return new GemPacket(*this);
}

int GemPacket::Type() {
    return g_nGemPacketType;
}

const char *GemPacket::GetName() const {
    return "GemPacket";
}

void GemPacket::PrintExtra(std::ostream &stream) const {
    mFields.Print(stream);
    stream << " tr:" << mTr << " clid:" << mClientId;
}

void GemPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mFields.saveGuts(stream);

    int tr = mTr;
    int clientId = mClientId;
    stream.WriteLE(&tr, sizeof(tr)).WriteLE(&clientId, sizeof(clientId));
}

void GemPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mFields.restoreGuts(stream);
    stream.ReadLE(&mTr, sizeof(mTr)).ReadLE(&mClientId, sizeof(mClientId));
}
