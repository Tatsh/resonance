#include "msg/updatescorepacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *UpdateScorePacket::New() {
    return new UpdateScorePacket;
}

Message *UpdateScorePacket::Clone() {
    // The copy constructor at 0x003f3818 is the compiler expanding the implicit one.
    return new UpdateScorePacket(*this);
}

int UpdateScorePacket::Type() {
    return g_nUpdateScorePacketType;
}

const char *UpdateScorePacket::GetName() const {
    return "UpdateScorePacket";
}

void UpdateScorePacket::PrintExtra(std::ostream &stream) const {
    stream << "pid:" << mPlayerId << " score-delta:" << mScoreDelta;
}

void UpdateScorePacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int playerId = mPlayerId;
    stream.WriteLE(&playerId, sizeof(playerId));

    int scoreDelta = mScoreDelta;
    stream.WriteLE(&scoreDelta, sizeof(scoreDelta));
}

void UpdateScorePacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    stream.ReadLE(&mPlayerId, sizeof(mPlayerId));
    stream.ReadLE(&mScoreDelta, sizeof(mScoreDelta));
}
