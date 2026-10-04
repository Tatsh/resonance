#include "msg/bumppacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *BumpPacket::New() {
    return new BumpPacket;
}

Message *BumpPacket::Clone() {
    // The copy constructor at 0x003f3b58 is the compiler expanding the implicit one.
    return new BumpPacket(*this);
}

int BumpPacket::Type() {
    return g_nBumpPacketType;
}

const char *BumpPacket::GetName() const {
    return "BumpPacket";
}

void BumpPacket::PrintExtra(std::ostream &stream) const {
    stream << static_cast<void *>(static_cast<Player *>(mPlayer)) << " bar " << mBar << " track "
           << mTrack;
}

void BumpPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int id = mPlayer.mId;
    int bar = mBar;
    int track = mTrack;
    stream.WriteLE(&id, sizeof(id)).WriteLE(&bar, sizeof(bar)).WriteLE(&track, sizeof(track));
}

void BumpPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    stream.ReadLE(&mPlayer.mId, sizeof(mPlayer.mId))
        .ReadLE(&mBar, sizeof(mBar))
        .ReadLE(&mTrack, sizeof(mTrack));
}
