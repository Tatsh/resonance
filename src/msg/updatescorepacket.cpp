#include "msg/updatescorepacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e5268
Message *UpdateScorePacket::New() {
    return new UpdateScorePacket;
}

// 0x003f06c0. Clone allocates and hands off to the copy constructor at 0x003f3818, which is
// the compiler expanding the implicit one.
Message *UpdateScorePacket::Clone() {
    return new UpdateScorePacket(*this);
}

// 0x003f0738
int UpdateScorePacket::Type() {
    return g_nUpdateScorePacketType;
}

// 0x003f0748
const char *UpdateScorePacket::Name() {
    return "UpdateScorePacket";
}

// 0x003f2510
void UpdateScorePacket::Print(std::ostream &stream) {
    stream << "pid:" << mPlayerId << " score-delta:" << mScoreDelta;
}

// 0x003e7018
void UpdateScorePacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int playerId = mPlayerId;
    stream.Write(&playerId, sizeof(playerId));

    int scoreDelta = mScoreDelta;
    stream.Write(&scoreDelta, sizeof(scoreDelta));
}

// 0x003e7120
void UpdateScorePacket::Load(IBStream &stream) {
    Packet::Load(stream);
    stream.Read(&mPlayerId, sizeof(mPlayerId));
    stream.Read(&mScoreDelta, sizeof(mScoreDelta));
}
