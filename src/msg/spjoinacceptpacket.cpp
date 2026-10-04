#include "msg/spjoinacceptpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

SPJoinAcceptPacket::SPJoinAcceptPacket() {
}

Message *SPJoinAcceptPacket::New() {
    return new SPJoinAcceptPacket;
}

Message *SPJoinAcceptPacket::Clone() {
    // The copy constructor at 0x003f2e48 is the compiler expanding the implicit one.
    return new SPJoinAcceptPacket(*this);
}

int SPJoinAcceptPacket::Type() {
    return g_nSPJoinAcceptPacketType;
}

const char *SPJoinAcceptPacket::GetName() const {
    return "SPJoinAcceptPacket";
}

void SPJoinAcceptPacket::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << " plid:" << mPlayerId << " destid:" << mDestId;
    mParams.Print(rest);
    mAppearance.Print(rest << " clr:" << mColorName << " thm:");
}

void SPJoinAcceptPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int playerId = mPlayerId;
    int destId = mDestId;
    OBStream &rest = stream.WriteLE(&playerId, sizeof(playerId)).WriteLE(&destId, sizeof(destId));
    mParams.Save(&rest);

    OBStream &tail = SaveHxStr(rest, mColorName);
    mAppearance.Save(tail);

    int count = static_cast<int>(mPlayers.size());
    tail.WriteLE(&count, sizeof(count));
    for (auto &info : mPlayers) {
        info.Save(tail);
    }
}

void SPJoinAcceptPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    IBStream &rest = stream.ReadLE(&mPlayerId, sizeof(mPlayerId)).ReadLE(&mDestId, sizeof(mDestId));
    mParams.Load(&rest);

    IBStream &tail = LoadHxStr(rest, mColorName);
    mAppearance.Load(tail);

    int count;
    tail.ReadLE(&count, sizeof(count));
    mPlayers.resize(count);
    for (auto &info : mPlayers) {
        info.Load(tail);
    }
}

HxStr SPJoinAcceptPacket::GetUsername() {
    return mAppearance.mUserName;
}

GameParams SPJoinAcceptPacket::GetParams() {
    return mParams;
}

HxStr SPJoinAcceptPacket::GetColorName() {
    return mColorName;
}

FreqAppearance SPJoinAcceptPacket::GetAppearance() {
    return mAppearance;
}

void SPJoinAcceptPacket::AddPlayer(const PlayerInfo &player) {
    mPlayers.push_back(player);
}
