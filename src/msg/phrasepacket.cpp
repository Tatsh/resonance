#include "msg/phrasepacket.h"

#include <iostream>

#include "game/phrase.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *PhrasePacket::New() {
    return new PhrasePacket;
}

Message *PhrasePacket::Clone() {
    // The copy constructor at 0x003f3760 is the compiler expanding the implicit one.
    return new PhrasePacket(*this);
}

int PhrasePacket::Type() {
    return g_nPhrasePacketType;
}

const char *PhrasePacket::GetName() const {
    return "PhrasePacket";
}

void PhrasePacket::PrintExtra(std::ostream &stream) const {
    stream << "tr:" << mTr << " b:" << mB << " ";
    if (mPhrase != nullptr) {
        stream << *mPhrase;
    } else {
        stream << "[empty]";
    }
}

void PhrasePacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    unsigned int tr = mTr;
    int b = mB;
    int clientId = mClientId;
    (stream.WriteLE(&tr, sizeof(tr)).WriteLE(&b, sizeof(b)) << mPhrase)
        .WriteLE(&clientId, sizeof(clientId));
}

void PhrasePacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    (stream.ReadLE(&mTr, sizeof(mTr)).ReadLE(&mB, sizeof(mB)) >> mPhrase)
        .ReadLE(&mClientId, sizeof(mClientId));
}
