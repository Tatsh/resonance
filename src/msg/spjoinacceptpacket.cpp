#include "msg/spjoinacceptpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003ef110
SPJoinAcceptPacket::SPJoinAcceptPacket() {
}

// 0x003e4b40
Message *SPJoinAcceptPacket::New() {
    return new SPJoinAcceptPacket;
}

// 0x003ef078. Clone allocates and hands off to the copy constructor at 0x003f2e48, which is
// the compiler expanding the implicit one.
Message *SPJoinAcceptPacket::Clone() {
    return new SPJoinAcceptPacket(*this);
}

// 0x003ef0f0
int SPJoinAcceptPacket::Type() {
    return g_nSPJoinAcceptPacketType;
}

// 0x003ef100
const char *SPJoinAcceptPacket::Name() {
    return "SPJoinAcceptPacket";
}

// 0x003f1f58
void SPJoinAcceptPacket::Print(std::ostream &stream) {
    std::ostream &rest = stream << " plid:" << mPlayerId << " destid:" << mDestId;
    mUnknown1c.Print(rest);
    mUnknown5c.Print(rest << " clr:" << mColorName << " thm:");
}

// 0x003e5718
void SPJoinAcceptPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int playerId = mPlayerId;
    int destId = mDestId;
    OBStream &rest = stream.Write(&playerId, sizeof(playerId)).Write(&destId, sizeof(destId));
    mUnknown1c.Save(&rest);

    OBStream &tail = SaveHxStr(rest, mColorName);
    mUnknown5c.Save(tail);

    int count = static_cast<int>(mUnknown70.size());
    tail.Write(&count, sizeof(count));
    for (auto &info : mUnknown70) {
        info.Save(tail);
    }
}

// 0x003e5930
void SPJoinAcceptPacket::Load(IBStream &stream) {
    Packet::Load(stream);

    IBStream &rest = stream.Read(&mPlayerId, sizeof(mPlayerId)).Read(&mDestId, sizeof(mDestId));
    mUnknown1c.Load(&rest);

    IBStream &tail = LoadHxStr(rest, mColorName);
    mUnknown5c.Load(tail);

    int count;
    tail.Read(&count, sizeof(count));
    mUnknown70.resize(count);
    for (auto &info : mUnknown70) {
        info.Load(tail);
    }
}
