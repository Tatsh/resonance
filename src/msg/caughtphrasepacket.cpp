#include "msg/caughtphrasepacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e5200
Message *CaughtPhrasePacket::New() {
    return new CaughtPhrasePacket;
}

// 0x003f04b8. Clone allocates and hands off to the copy constructor at 0x003f37b8, which is
// the compiler expanding the implicit one.
Message *CaughtPhrasePacket::Clone() {
    return new CaughtPhrasePacket(*this);
}

// 0x003f0530
int CaughtPhrasePacket::Type() {
    return g_nCaughtPhrasePacketType;
}

// 0x003f0540
const char *CaughtPhrasePacket::Name() {
    return "CaughtPhrasePacket";
}

// 0x003f24a8
void CaughtPhrasePacket::Print(std::ostream &stream) {
    stream << " tr:" << mTr << " b:" << mB << " ";
}

// 0x003e6db0. The word at +0x0c crosses the wire twice.
void CaughtPhrasePacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int id = mPlayer.mId;
    unsigned int tr = mTr;
    int b = mB;
    int unknown0c = mUnknown0c;
    stream.Write(&id, sizeof(id))
        .Write(&tr, sizeof(tr))
        .Write(&b, sizeof(b))
        .Write(&unknown0c, sizeof(unknown0c));
}

// 0x003e6f00
void CaughtPhrasePacket::Load(IBStream &stream) {
    Packet::Load(stream);
    stream.Read(&mPlayer.mId, sizeof(mPlayer.mId))
        .Read(&mTr, sizeof(mTr))
        .Read(&mB, sizeof(mB))
        .Read(&mUnknown0c, sizeof(mUnknown0c));
}
