#include "msg/bumppacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e5410, PAL: 0x0041d6a8
Message *BumpPacket::New() {
    return new BumpPacket;
}

// NTSC-U/C: 0x003f0e80, PAL: 0x00429488
// Clone allocates and hands off to the copy constructor at 0x003f3b58, which is
// the compiler expanding the implicit one.
Message *BumpPacket::Clone() {
    return new BumpPacket(*this);
}

// NTSC-U/C: 0x003f0ef8, PAL: 0x00429500
int BumpPacket::Type() {
    return g_nBumpPacketType;
}

// NTSC-U/C: 0x003f0f08, PAL: 0x00429510
const char *BumpPacket::GetName() const {
    return "BumpPacket";
}

// NTSC-U/C: 0x003f26d8, PAL: 0x0042ac20
void BumpPacket::PrintExtra(std::ostream &stream) const {
    stream << static_cast<void *>(static_cast<Player *>(mPlayer)) << " bar " << mBar << " track "
           << mTrack;
}

// NTSC-U/C: 0x003e7d88, PAL: 0x00420068
void BumpPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int id = mPlayer.mId;
    int bar = mBar;
    int track = mTrack;
    stream.Write(&id, sizeof(id)).Write(&bar, sizeof(bar)).Write(&track, sizeof(track));
}

// NTSC-U/C: 0x003e7eb0, PAL: 0x00420190
void BumpPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    stream.Read(&mPlayer.mId, sizeof(mPlayer.mId))
        .Read(&mBar, sizeof(mBar))
        .Read(&mTrack, sizeof(mTrack));
}
