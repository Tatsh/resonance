#include "msg/caughtphrasepacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e5200, PAL: 0x0041d498
Message *CaughtPhrasePacket::New() {
    return new CaughtPhrasePacket;
}

// NTSC-U/C: 0x003f04b8, PAL: 0x00428ac0
// Clone allocates and hands off to the copy constructor at 0x003f37b8, which is
// the compiler expanding the implicit one.
Message *CaughtPhrasePacket::Clone() {
    return new CaughtPhrasePacket(*this);
}

// NTSC-U/C: 0x003f0530, PAL: 0x00428b38
int CaughtPhrasePacket::Type() {
    return g_nCaughtPhrasePacketType;
}

// NTSC-U/C: 0x003f0540, PAL: 0x00428b48
const char *CaughtPhrasePacket::GetName() const {
    return "CaughtPhrasePacket";
}

// NTSC-U/C: 0x003f24a8, PAL: 0x0042a9f0
void CaughtPhrasePacket::PrintExtra(std::ostream &stream) const {
    stream << " tr:" << mTr << " b:" << mB << " ";
}

// NTSC-U/C: 0x003e6db0, PAL: 0x0041f090
// The word at +0x0c crosses the wire twice.
void CaughtPhrasePacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int id = mPlayer.mId;
    unsigned int tr = mTr;
    int b = mB;
    int clientId = mClientId;
    stream.Write(&id, sizeof(id))
        .Write(&tr, sizeof(tr))
        .Write(&b, sizeof(b))
        .Write(&clientId, sizeof(clientId));
}

// NTSC-U/C: 0x003e6f00, PAL: 0x0041f1e0
void CaughtPhrasePacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    stream.Read(&mPlayer.mId, sizeof(mPlayer.mId))
        .Read(&mTr, sizeof(mTr))
        .Read(&mB, sizeof(mB))
        .Read(&mClientId, sizeof(mClientId));
}
