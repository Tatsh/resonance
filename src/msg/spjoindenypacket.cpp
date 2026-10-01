#include "msg/spjoindenypacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003ef628
SPJoinDenyPacket::SPJoinDenyPacket(int nReasonCode, const HxStr &reason)
    : mReasonCode(nReasonCode), mReason(reason) {
}

// 0x003e4ca0
Message *SPJoinDenyPacket::New() {
    return new SPJoinDenyPacket;
}

// 0x003ef558
// Clone allocates and hands off to the copy constructor at 0x003f3130, which is
// the compiler expanding the implicit one.
Message *SPJoinDenyPacket::Clone() {
    return new SPJoinDenyPacket(*this);
}

// 0x003ef5d0
int SPJoinDenyPacket::Type() {
    return g_nSPJoinDenyPacketType;
}

// 0x003ef5e0
const char *SPJoinDenyPacket::Name() {
    return "SPJoinDenyPacket";
}

// 0x003f2000
void SPJoinDenyPacket::Print(std::ostream &stream) {
    stream << mReasonCode << " " << mReason;
}

// 0x003e5d58
void SPJoinDenyPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int reasonCode = mReasonCode;
    SaveHxStr(stream.Write(&reasonCode, sizeof(reasonCode)), mReason);
}

// 0x003e5e98
void SPJoinDenyPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    LoadHxStr(stream.Read(&mReasonCode, sizeof(mReasonCode)), mReason);
}
