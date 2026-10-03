#include "msg/spjoindenypacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003ef628, PAL: 0x00427c18
SPJoinDenyPacket::SPJoinDenyPacket(int nReasonCode, const HxStr &reason)
    : mReasonCode(nReasonCode), mReason(reason) {
}

// NTSC-U/C: 0x003e4ca0, PAL: 0x0041cf18
Message *SPJoinDenyPacket::New() {
    return new SPJoinDenyPacket;
}

// NTSC-U/C: 0x003ef558, PAL: 0x00427b40
// Clone allocates and hands off to the copy constructor at 0x003f3130, which is
// the compiler expanding the implicit one.
Message *SPJoinDenyPacket::Clone() {
    return new SPJoinDenyPacket(*this);
}

// NTSC-U/C: 0x003ef5d0, PAL: 0x00427bb8
int SPJoinDenyPacket::Type() {
    return g_nSPJoinDenyPacketType;
}

// NTSC-U/C: 0x003ef5e0, PAL: 0x00427bc8
const char *SPJoinDenyPacket::GetName() const {
    return "SPJoinDenyPacket";
}

// NTSC-U/C: 0x003f2000, PAL: 0x0042a548
void SPJoinDenyPacket::PrintExtra(std::ostream &stream) const {
    stream << mReasonCode << " " << mReason;
}

// NTSC-U/C: 0x003e5d58, PAL: 0x0041e038
void SPJoinDenyPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int reasonCode = mReasonCode;
    SaveHxStr(stream.WriteLE(&reasonCode, sizeof(reasonCode)), mReason);
}

// NTSC-U/C: 0x003e5e98, PAL: 0x0041e178
void SPJoinDenyPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    LoadHxStr(stream.ReadLE(&mReasonCode, sizeof(mReasonCode)), mReason);
}
