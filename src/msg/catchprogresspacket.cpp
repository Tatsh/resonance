#include "msg/catchprogresspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e5330
Message *CatchProgressPacket::New() {
    return new CatchProgressPacket;
}

// 0x003f0a70
// Clone allocates and hands off to the copy constructor at 0x003f38d0, which is
// the compiler expanding the implicit one.
Message *CatchProgressPacket::Clone() {
    return new CatchProgressPacket(*this);
}

// 0x003f0ae8
int CatchProgressPacket::Type() {
    return g_nCatchProgressPacketType;
}

// 0x003f0af8
const char *CatchProgressPacket::Name() {
    return "CatchProgressPacket";
}

// 0x003f2648
void CatchProgressPacket::Print(std::ostream &stream) {
    std::ostream &rest = stream << " @";
    mPosition.Print(rest);
    rest << " track:" << mTrack << " succ:" << mSucc;
}

// 0x003e7430
// The stream Mid::MBT::Save() returns is not used.
void CatchProgressPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int id = mPlayer.mId;
    OBStream &rest = stream.Write(&id, sizeof(id));
    mPosition.Save(rest);

    int track = mTrack;
    float succ = mSucc;
    rest.Write(&track, sizeof(track)).Write(&succ, sizeof(succ));
}

// 0x003e7568
void CatchProgressPacket::Load(IBStream &stream) {
    Packet::Load(stream);

    IBStream &rest = stream.Read(&mPlayer.mId, sizeof(mPlayer.mId));
    mPosition.Load(rest);
    rest.Read(&mTrack, sizeof(mTrack)).Read(&mSucc, sizeof(mSucc));
}
