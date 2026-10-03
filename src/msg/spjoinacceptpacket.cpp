#include "msg/spjoinacceptpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003ef110, PAL: 0x00427660
SPJoinAcceptPacket::SPJoinAcceptPacket() {
}

// NTSC-U/C: 0x003e4b40, PAL: 0x0041cd70
Message *SPJoinAcceptPacket::New() {
    return new SPJoinAcceptPacket;
}

// NTSC-U/C: 0x003ef078, PAL: 0x004275c8
// Clone allocates and hands off to the copy constructor at 0x003f2e48, which is
// the compiler expanding the implicit one.
Message *SPJoinAcceptPacket::Clone() {
    return new SPJoinAcceptPacket(*this);
}

// NTSC-U/C: 0x003ef0f0, PAL: 0x00427640
int SPJoinAcceptPacket::Type() {
    return g_nSPJoinAcceptPacketType;
}

// NTSC-U/C: 0x003ef100, PAL: 0x00427650
const char *SPJoinAcceptPacket::GetName() const {
    return "SPJoinAcceptPacket";
}

// NTSC-U/C: 0x003f1f58, PAL: 0x0042a4a0
void SPJoinAcceptPacket::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << " plid:" << mPlayerId << " destid:" << mDestId;
    mParams.Print(rest);
    mAppearance.Print(rest << " clr:" << mColorName << " thm:");
}

// NTSC-U/C: 0x003e5718, PAL: 0x0041d9c0
void SPJoinAcceptPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int playerId = mPlayerId;
    int destId = mDestId;
    OBStream &rest = stream.Write(&playerId, sizeof(playerId)).Write(&destId, sizeof(destId));
    mParams.Save(&rest);

    OBStream &tail = SaveHxStr(rest, mColorName);
    mAppearance.Save(tail);

    int count = static_cast<int>(mPlayers.size());
    tail.Write(&count, sizeof(count));
    for (auto &info : mPlayers) {
        info.Save(tail);
    }
}

// NTSC-U/C: 0x003e5930, PAL: 0x0041dbd8
void SPJoinAcceptPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    IBStream &rest = stream.Read(&mPlayerId, sizeof(mPlayerId)).Read(&mDestId, sizeof(mDestId));
    mParams.Load(&rest);

    IBStream &tail = LoadHxStr(rest, mColorName);
    mAppearance.Load(tail);

    int count;
    tail.Read(&count, sizeof(count));
    mPlayers.resize(count);
    for (auto &info : mPlayers) {
        info.Load(tail);
    }
}

// NTSC-U/C: 0x003ef390, PAL: 0x00427968
HxStr SPJoinAcceptPacket::GetUsername() {
    return mAppearance.mUserName;
}

// NTSC-U/C: 0x003ef3c0, PAL: 0x00427998
GameParams SPJoinAcceptPacket::GetParams() {
    return mParams;
}

// NTSC-U/C: 0x003ef3f0, PAL: 0x004279c8
HxStr SPJoinAcceptPacket::GetColorName() {
    return mColorName;
}

// NTSC-U/C: 0x003ef420, PAL: 0x004279f8
FreqAppearance SPJoinAcceptPacket::GetAppearance() {
    return mAppearance;
}

// NTSC-U/C: 0x003ef450, PAL: 0x00427a28
void SPJoinAcceptPacket::AddPlayer(const PlayerInfo &player) {
    mPlayers.push_back(player);
}
