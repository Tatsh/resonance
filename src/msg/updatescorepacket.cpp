#include "msg/updatescorepacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e5268, PAL: 0x0041d500
Message *UpdateScorePacket::New() {
    return new UpdateScorePacket;
}

// NTSC-U/C: 0x003f06c0, PAL: 0x00428cc8
// Clone allocates and hands off to the copy constructor at 0x003f3818, which is
// the compiler expanding the implicit one.
Message *UpdateScorePacket::Clone() {
    return new UpdateScorePacket(*this);
}

// NTSC-U/C: 0x003f0738, PAL: 0x00428d40
int UpdateScorePacket::Type() {
    return g_nUpdateScorePacketType;
}

// NTSC-U/C: 0x003f0748, PAL: 0x00428d50
const char *UpdateScorePacket::GetName() const {
    return "UpdateScorePacket";
}

// NTSC-U/C: 0x003f2510, PAL: 0x0042aa58
void UpdateScorePacket::PrintExtra(std::ostream &stream) const {
    stream << "pid:" << mPlayerId << " score-delta:" << mScoreDelta;
}

// NTSC-U/C: 0x003e7018, PAL: 0x0041f2f8
void UpdateScorePacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int playerId = mPlayerId;
    stream.Write(&playerId, sizeof(playerId));

    int scoreDelta = mScoreDelta;
    stream.Write(&scoreDelta, sizeof(scoreDelta));
}

// NTSC-U/C: 0x003e7120, PAL: 0x0041f400
void UpdateScorePacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    stream.Read(&mPlayerId, sizeof(mPlayerId));
    stream.Read(&mScoreDelta, sizeof(mScoreDelta));
}
