#include "msg/phrasepacket.h"

#include <iostream>

#include "game/phrase.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e51a8
Message *PhrasePacket::New() {
    return new PhrasePacket;
}

// 0x003f0320. Clone allocates and hands off to the copy constructor at 0x003f3760, which is
// the compiler expanding the implicit one.
Message *PhrasePacket::Clone() {
    return new PhrasePacket(*this);
}

// 0x003f0398
int PhrasePacket::Type() {
    return g_nPhrasePacketType;
}

// 0x003f03a8
const char *PhrasePacket::Name() {
    return "PhrasePacket";
}

// 0x003f2408
void PhrasePacket::Print(std::ostream &stream) {
    stream << "tr:" << mTr << " b:" << mB << " ";
    if (mPhrase != nullptr) {
        stream << *mPhrase;
    } else {
        stream << "[empty]";
    }
}

// 0x003e6b70. The word at +0x0c crosses the wire twice.
void PhrasePacket::Save(OBStream &stream) {
    Packet::Save(stream);

    unsigned int tr = mTr;
    int b = mB;
    int unknown0c = mUnknown0c;
    (stream.Write(&tr, sizeof(tr)).Write(&b, sizeof(b)) << mPhrase)
        .Write(&unknown0c, sizeof(unknown0c));
}

// 0x003e6ca8
void PhrasePacket::Load(IBStream &stream) {
    Packet::Load(stream);
    (stream.Read(&mTr, sizeof(mTr)).Read(&mB, sizeof(mB)) >> mPhrase)
        .Read(&mUnknown0c, sizeof(mUnknown0c));
}
