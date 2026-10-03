#include "msg/phrasepacket.h"

#include <iostream>

#include "game/phrase.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e51a8, PAL: 0x0041d440
Message *PhrasePacket::New() {
    return new PhrasePacket;
}

// NTSC-U/C: 0x003f0320, PAL: 0x00428928
// Clone allocates and hands off to the copy constructor at 0x003f3760, which is
// the compiler expanding the implicit one.
Message *PhrasePacket::Clone() {
    return new PhrasePacket(*this);
}

// NTSC-U/C: 0x003f0398, PAL: 0x004289a0
int PhrasePacket::Type() {
    return g_nPhrasePacketType;
}

// NTSC-U/C: 0x003f03a8, PAL: 0x004289b0
const char *PhrasePacket::GetName() const {
    return "PhrasePacket";
}

// NTSC-U/C: 0x003f2408, PAL: 0x0042a950
void PhrasePacket::PrintExtra(std::ostream &stream) const {
    stream << "tr:" << mTr << " b:" << mB << " ";
    if (mPhrase != nullptr) {
        stream << *mPhrase;
    } else {
        stream << "[empty]";
    }
}

// NTSC-U/C: 0x003e6b70, PAL: 0x0041ee50
// The word at +0x0c crosses the wire twice.
void PhrasePacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    unsigned int tr = mTr;
    int b = mB;
    int clientId = mClientId;
    (stream.Write(&tr, sizeof(tr)).Write(&b, sizeof(b)) << mPhrase)
        .Write(&clientId, sizeof(clientId));
}

// NTSC-U/C: 0x003e6ca8, PAL: 0x0041ef88
void PhrasePacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    (stream.Read(&mTr, sizeof(mTr)).Read(&mB, sizeof(mB)) >> mPhrase)
        .Read(&mClientId, sizeof(mClientId));
}
