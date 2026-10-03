#include "msg/catchprogresspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e5330, PAL: 0x0041d5c8
Message *CatchProgressPacket::New() {
    return new CatchProgressPacket;
}

// NTSC-U/C: 0x003f0a70, PAL: 0x00429078
// Clone allocates and hands off to the copy constructor at 0x003f38d0, which is
// the compiler expanding the implicit one.
Message *CatchProgressPacket::Clone() {
    return new CatchProgressPacket(*this);
}

// NTSC-U/C: 0x003f0ae8, PAL: 0x004290f0
int CatchProgressPacket::Type() {
    return g_nCatchProgressPacketType;
}

// NTSC-U/C: 0x003f0af8, PAL: 0x00429100
const char *CatchProgressPacket::GetName() const {
    return "CatchProgressPacket";
}

// NTSC-U/C: 0x003f2648, PAL: 0x0042ab90
void CatchProgressPacket::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << " @";
    mPosition.Print(rest);
    rest << " track:" << mTrack << " succ:" << mSucc;
}

// NTSC-U/C: 0x003e7430, PAL: 0x0041f710
// The stream Mid::MBT::Save() returns is not used.
void CatchProgressPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int id = mPlayer.mId;
    OBStream &rest = stream.Write(&id, sizeof(id));
    mPosition.Save(rest);

    int track = mTrack;
    float succ = mSucc;
    rest.Write(&track, sizeof(track)).Write(&succ, sizeof(succ));
}

// NTSC-U/C: 0x003e7568, PAL: 0x0041f848
void CatchProgressPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    IBStream &rest = stream.Read(&mPlayer.mId, sizeof(mPlayer.mId));
    mPosition.Load(rest);
    rest.Read(&mTrack, sizeof(mTrack)).Read(&mSucc, sizeof(mSucc));
}
