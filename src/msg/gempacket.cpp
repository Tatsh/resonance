#include "msg/gempacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e5148 allocates 0x2c bytes against the `MSG` tag and expands this constructor into the
// allocation. The four words ahead of the payload take 2, 3, -1, and -1 there, which the Packet
// prefix supplies rather than this class.
GemPacket::GemPacket() {
    mFields.mLoc.mTick = kMBTInfinity;
}

// 0x003e5148. Registered against g_nGemPacketType by the translation unit at 0x003ed2e0.
Message *GemPacket::New() {
    return new GemPacket();
}

// 0x003f1750. Clone allocates and hands off to the copy constructor at 0x003f3d18, which is the
// compiler expanding the implicit one.
Message *GemPacket::Clone() {
    return new GemPacket(*this);
}

// 0x003f17c8
int GemPacket::Type() {
    return g_nGemPacketType;
}

// 0x003f17d8
const char *GemPacket::Name() {
    return "GemPacket";
}

// 0x003f2878
void GemPacket::Print(std::ostream &stream) {
    mFields.Print(stream);
    stream << " tr:" << mTr << " clid:" << mUnknown0c;
}

// 0x003e8258. The transfer of mUnknown0c repeats the one the Packet prefix already performed, and
// Load() reads the same word twice to match, so the word crosses the wire twice.
void GemPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    mFields.Save(stream);
    stream << mTr;
    stream << mUnknown0c;
}

// 0x003e8368
void GemPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    mFields.Load(stream);
    stream >> mTr;
    stream >> mUnknown0c;
}

// 0x001a2560
void GemPacket::Fields::Save(OBStream &stream) {
    stream << mGem << mTrans << mBar;
    mLoc.Save(stream);
    stream << mPlayer->mId20;
}

// 0x001a2ce0
void GemPacket::Fields::Print(std::ostream &stream) {
    stream << "gem: " << mGem << " trans:" << mTrans << " bar:" << mBar << " loc:";
    mLoc.Print(stream);
    stream << " pid:" << mPlayer->mId20;
}
