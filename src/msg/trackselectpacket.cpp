#include "msg/trackselectpacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e52c0
Message *TrackSelectPacket::New() {
    return new TrackSelectPacket;
}

// 0x003f0848. Clone allocates and hands off to the copy constructor at 0x003f3868, which is
// the compiler expanding the implicit one.
Message *TrackSelectPacket::Clone() {
    return new TrackSelectPacket(*this);
}

// 0x003f08c0
int TrackSelectPacket::Type() {
    return g_nTrackSelectPacketType;
}

// 0x003f08d0
const char *TrackSelectPacket::Name() {
    return "TrackSelectPacket";
}

// 0x003f2568
void TrackSelectPacket::Print(std::ostream &stream) {
    mPosition.Print(stream);
    std::ostream &rest = stream << " ";
    static_cast<Player *>(mPlayer)->Print(rest);
    rest << " track:" << mTrack << " place:" << mPlace;
}

// 0x003e71f8. The stream Mid::MBT::Save() returns is not used.
void TrackSelectPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    mPosition.Save(stream);

    int id = mPlayer.mId;
    int track = mTrack;
    int place = mPlace;
    stream.Write(&id, sizeof(id)).Write(&track, sizeof(track)).Write(&place, sizeof(place));
}

// 0x003e7330
void TrackSelectPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    mPosition.Load(stream);
    stream.Read(&mPlayer.mId, sizeof(mPlayer.mId))
        .Read(&mTrack, sizeof(mTrack))
        .Read(&mPlace, sizeof(mPlace));
}
