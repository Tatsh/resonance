#include "msg/catchprogresspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *CatchProgressPacket::New() {
    return new CatchProgressPacket;
}

Message *CatchProgressPacket::Clone() {
    // The copy constructor at 0x003f38d0 is the compiler expanding the implicit one.
    return new CatchProgressPacket(*this);
}

int CatchProgressPacket::Type() {
    return g_nCatchProgressPacketType;
}

const char *CatchProgressPacket::GetName() const {
    return "CatchProgressPacket";
}

void CatchProgressPacket::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << " @";
    mPosition.Print(rest);
    rest << " track:" << mTrack << " succ:" << mSucc;
}

void CatchProgressPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int id = mPlayer.mId;
    OBStream &rest = stream.WriteLE(&id, sizeof(id));
    mPosition.saveGuts(rest); // The stream Sch::Tick::saveGuts() returns is not used.

    int track = mTrack;
    float succ = mSucc;
    rest.WriteLE(&track, sizeof(track)).WriteLE(&succ, sizeof(succ));
}

void CatchProgressPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    IBStream &rest = stream.ReadLE(&mPlayer.mId, sizeof(mPlayer.mId));
    mPosition.restoreGuts(rest);
    rest.ReadLE(&mTrack, sizeof(mTrack)).ReadLE(&mSucc, sizeof(mSucc));
}
