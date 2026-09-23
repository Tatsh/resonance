#include "msg/bumppacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e5410
Message *BumpPacket::New() {
    return new BumpPacket;
}

// 0x003f0e80. Clone allocates and hands off to the copy constructor at 0x003f3b58, which is
// the compiler expanding the implicit one.
Message *BumpPacket::Clone() {
    return new BumpPacket(*this);
}

// 0x003f0ef8
int BumpPacket::Type() {
    return g_nBumpPacketType;
}

// 0x003f0f08
const char *BumpPacket::Name() {
    return "BumpPacket";
}

// 0x003f26d8
void BumpPacket::Print(std::ostream &stream) {
    stream << static_cast<void *>(static_cast<Player *>(mPlayer)) << " bar " << mBar << " track "
           << mTrack;
}

// 0x003e7d88
void BumpPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int id = mPlayer.mId;
    int bar = mBar;
    int track = mTrack;
    stream.Write(&id, sizeof(id)).Write(&bar, sizeof(bar)).Write(&track, sizeof(track));
}

// 0x003e7eb0
void BumpPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    stream.Read(&mPlayer.mId, sizeof(mPlayer.mId))
        .Read(&mBar, sizeof(mBar))
        .Read(&mTrack, sizeof(mTrack));
}
