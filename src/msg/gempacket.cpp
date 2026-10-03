#include "msg/gempacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e5148, PAL: 0x0041d3e0
// Registered against g_nGemPacketType by the translation unit at 0x003ed2e0.
Message *GemPacket::New() {
    return new GemPacket;
}

// NTSC-U/C: 0x003f1750, PAL: 0x00429c18
// Clone allocates and hands off to the copy constructor at 0x003f3d18, which is the
// compiler expanding the implicit one.
Message *GemPacket::Clone() {
    return new GemPacket(*this);
}

// NTSC-U/C: 0x003f17c8, PAL: 0x00429c90
int GemPacket::Type() {
    return g_nGemPacketType;
}

// NTSC-U/C: 0x003f17d8, PAL: 0x00429ca0
const char *GemPacket::GetName() const {
    return "GemPacket";
}

// NTSC-U/C: 0x003f2878, PAL: 0x0042adc0
void GemPacket::PrintExtra(std::ostream &stream) const {
    mFields.Print(stream);
    stream << " tr:" << mTr << " clid:" << mClientId;
}

// NTSC-U/C: 0x003e8258, PAL: 0x00420538
// The transfer of mClientId repeats the one the Packet prefix already performed, and
// restoreGuts() reads the same word twice to match.
void GemPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mFields.saveGuts(stream);

    int tr = mTr;
    int clientId = mClientId;
    stream.Write(&tr, sizeof(tr)).Write(&clientId, sizeof(clientId));
}

// NTSC-U/C: 0x003e8368, PAL: 0x00420648
void GemPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mFields.restoreGuts(stream);
    stream.Read(&mTr, sizeof(mTr)).Read(&mClientId, sizeof(mClientId));
}
