#include "msg/caughtphrasepacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *CaughtPhrasePacket::New() {
    return new CaughtPhrasePacket;
}

Message *CaughtPhrasePacket::Clone() {
    // The copy constructor at 0x003f37b8 is the compiler expanding the implicit one.
    return new CaughtPhrasePacket(*this);
}

int CaughtPhrasePacket::Type() {
    return g_nCaughtPhrasePacketType;
}

const char *CaughtPhrasePacket::GetName() const {
    return "CaughtPhrasePacket";
}

void CaughtPhrasePacket::PrintExtra(std::ostream &stream) const {
    stream << " tr:" << mTr << " b:" << mB << " ";
}

void CaughtPhrasePacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int id = mPlayer.mId;
    unsigned int tr = mTr;
    int b = mB;
    int clientId = mClientId;
    stream.WriteLE(&id, sizeof(id))
        .WriteLE(&tr, sizeof(tr))
        .WriteLE(&b, sizeof(b))
        .WriteLE(&clientId, sizeof(clientId));
}

void CaughtPhrasePacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    stream.ReadLE(&mPlayer.mId, sizeof(mPlayer.mId))
        .ReadLE(&mTr, sizeof(mTr))
        .ReadLE(&mB, sizeof(mB))
        .ReadLE(&mClientId, sizeof(mClientId));
}
