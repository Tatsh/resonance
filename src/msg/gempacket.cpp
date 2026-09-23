#include "msg/gempacket.h"

#include <iostream>

#include "game/idableptr.h"
#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e5148
// Registered against g_nGemPacketType by the translation unit at 0x003ed2e0.
Message *GemPacket::New() {
    return new GemPacket;
}

// 0x003f1750
// Clone allocates and hands off to the copy constructor at 0x003f3d18, which is the
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

// 0x003e8258
// The transfer of mUnknown0c repeats the one the Packet prefix already performed, and
// Load() reads the same word twice to match, so the word crosses the wire twice.
void GemPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    mFields.Save(stream);

    int tr = mTr;
    int unknown0c = mUnknown0c;
    stream.Write(&tr, sizeof(tr)).Write(&unknown0c, sizeof(unknown0c));
}

// 0x003e8368
void GemPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    mFields.Load(stream);
    stream.Read(&mTr, sizeof(mTr)).Read(&mUnknown0c, sizeof(mUnknown0c));
}

// 0x001a2560
// The stream Mid::MBT::Save() returns is not used.
void GemPacket::Fields::Save(OBStream &stream) {
    int gem = mGem;
    int trans = mTrans;
    int bar = mBar;
    OBStream &rest =
        stream.Write(&gem, sizeof(gem)).Write(&trans, sizeof(trans)).Write(&bar, sizeof(bar));
    mLoc.Save(rest);

    int id = mPlayer->mId20;
    rest.Write(&id, sizeof(id));
}

// 0x001a2630
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

// 0x001a2ce0
void GemPacket::Fields::Print(std::ostream &stream) {
    stream << "gem: " << mGem << " trans:" << mTrans << " bar:" << mBar << " loc:";
    mLoc.Print(stream);
    stream << " pid:" << mPlayer->mId20;
}
