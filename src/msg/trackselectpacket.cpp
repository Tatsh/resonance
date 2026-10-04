#include "msg/trackselectpacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *TrackSelectPacket::New() {
    return new TrackSelectPacket;
}

Message *TrackSelectPacket::Clone() {
    // The copy constructor at 0x003f3868 is the compiler expanding the implicit one.
    return new TrackSelectPacket(*this);
}

int TrackSelectPacket::Type() {
    return g_nTrackSelectPacketType;
}

const char *TrackSelectPacket::GetName() const {
    return "TrackSelectPacket";
}

void TrackSelectPacket::PrintExtra(std::ostream &stream) const {
    mPosition.Print(stream);
    std::ostream &rest = stream << " ";
    static_cast<Player *>(mPlayer)->Print(rest);
    rest << " track:" << mTrack << " place:" << mPlace;
}

void TrackSelectPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mPosition.saveGuts(stream); // The returned stream is not used.

    int id = mPlayer.mId;
    int track = mTrack;
    int place = mPlace;
    stream.WriteLE(&id, sizeof(id)).WriteLE(&track, sizeof(track)).WriteLE(&place, sizeof(place));
}

void TrackSelectPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mPosition.restoreGuts(stream);
    stream.ReadLE(&mPlayer.mId, sizeof(mPlayer.mId))
        .ReadLE(&mTrack, sizeof(mTrack))
        .ReadLE(&mPlace, sizeof(mPlace));
}
