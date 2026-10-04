#include "msg/spjoindenypacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

SPJoinDenyPacket::SPJoinDenyPacket(int nReasonCode, const HxStr &reason)
    : mReasonCode(nReasonCode), mReason(reason) {
}

Message *SPJoinDenyPacket::New() {
    return new SPJoinDenyPacket;
}

Message *SPJoinDenyPacket::Clone() {
    // The copy constructor at 0x003f3130 is the compiler expanding the implicit one.
    return new SPJoinDenyPacket(*this);
}

int SPJoinDenyPacket::Type() {
    return g_nSPJoinDenyPacketType;
}

const char *SPJoinDenyPacket::GetName() const {
    return "SPJoinDenyPacket";
}

void SPJoinDenyPacket::PrintExtra(std::ostream &stream) const {
    stream << mReasonCode << " " << mReason;
}

void SPJoinDenyPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int reasonCode = mReasonCode;
    SaveHxStr(stream.WriteLE(&reasonCode, sizeof(reasonCode)), mReason);
}

void SPJoinDenyPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    LoadHxStr(stream.ReadLE(&mReasonCode, sizeof(mReasonCode)), mReason);
}
