#include "msg/gempacket.h"

#include <iostream>

#include "game/idableptr.h"
#include "game/player.h"
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
const char *GemPacket::Name() {
    return "GemPacket";
}

// NTSC-U/C: 0x003f2878, PAL: 0x0042adc0
void GemPacket::Print(std::ostream &stream) {
    mFields.Print(stream);
    stream << " tr:" << mTr << " clid:" << mClientId;
}

// NTSC-U/C: 0x003e8258, PAL: 0x00420538
// The transfer of mClientId repeats the one the Packet prefix already performed, and Load() reads
// the same word twice to match.
void GemPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    mFields.Save(stream);

    int tr = mTr;
    int clientId = mClientId;
    stream.Write(&tr, sizeof(tr)).Write(&clientId, sizeof(clientId));
}

// NTSC-U/C: 0x003e8368, PAL: 0x00420648
void GemPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    mFields.Load(stream);
    stream.Read(&mTr, sizeof(mTr)).Read(&mClientId, sizeof(mClientId));
}

// NTSC-U/C: 0x001a2560, PAL: 0x001a82c8
// The stream Mid::MBT::Save() returns is not used.
void GemPacket::Fields::Save(OBStream &stream) {
    int gem = mGem;
    int trans = mTrans;
    int bar = mBar;
    OBStream &rest =
        stream.Write(&gem, sizeof(gem)).Write(&trans, sizeof(trans)).Write(&bar, sizeof(bar));
    mLoc.Save(rest);

    int id = mPlayer->mPlayerId;
    rest.Write(&id, sizeof(id));
}

// NTSC-U/C: 0x001a2630, PAL: 0x001a8398
// The binary tests the local's cached pointer before the -1 case, and that pointer
// is always null here, so the order does not change the result.
void GemPacket::Fields::Load(IBStream &stream) {
    IBStream &rest =
        stream.Read(&mGem, sizeof(mGem)).Read(&mTrans, sizeof(mTrans)).Read(&mBar, sizeof(mBar));
    mLoc.Load(rest);

    IDablePtr<Player> player;
    rest.Read(&player.mId, sizeof(player.mId));
    mPlayer = player.mId == -1 ? nullptr : static_cast<Player *>(player);
}

// NTSC-U/C: 0x001a2ce0, PAL: 0x001a8a48
void GemPacket::Fields::Print(std::ostream &stream) {
    stream << "gem: " << mGem << " trans:" << mTrans << " bar:" << mBar << " loc:";
    mLoc.Print(stream);
    stream << " pid:" << mPlayer->mPlayerId;
}
