#include "msg/trackselectpacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e52c0, PAL: 0x0041d558
Message *TrackSelectPacket::New() {
    return new TrackSelectPacket;
}

// NTSC-U/C: 0x003f0848, PAL: 0x00428e50
// Clone allocates and hands off to the copy constructor at 0x003f3868, which is
// the compiler expanding the implicit one.
Message *TrackSelectPacket::Clone() {
    return new TrackSelectPacket(*this);
}

// NTSC-U/C: 0x003f08c0, PAL: 0x00428ec8
int TrackSelectPacket::Type() {
    return g_nTrackSelectPacketType;
}

// NTSC-U/C: 0x003f08d0, PAL: 0x00428ed8
const char *TrackSelectPacket::GetName() const {
    return "TrackSelectPacket";
}

// NTSC-U/C: 0x003f2568, PAL: 0x0042aab0
void TrackSelectPacket::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    std::ostream &rest = stream << " ";
    static_cast<Player *>(mPlayer)->Print(rest);
    rest << " track:" << mTrack << " place:" << mPlace;
}

// NTSC-U/C: 0x003e71f8, PAL: 0x0041f4d8
// The stream Sch::Tick::saveGuts() returns is not used.
void TrackSelectPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mPosition.saveGuts(stream);

    int id = mPlayer.mId;
    int track = mTrack;
    int place = mPlace;
    stream.WriteLE(&id, sizeof(id)).WriteLE(&track, sizeof(track)).WriteLE(&place, sizeof(place));
}

// NTSC-U/C: 0x003e7330, PAL: 0x0041f610
void TrackSelectPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mPosition.restoreGuts(stream);
    stream.ReadLE(&mPlayer.mId, sizeof(mPlayer.mId))
        .ReadLE(&mTrack, sizeof(mTrack))
        .ReadLE(&mPlace, sizeof(mPlace));
}
