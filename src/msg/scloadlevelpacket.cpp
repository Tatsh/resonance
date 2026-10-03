#include "msg/scloadlevelpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003f1438, PAL: 0x00429900
SCLoadLevelPacket::SCLoadLevelPacket() {
}

// NTSC-U/C: 0x003f14b0, PAL: 0x00429978
SCLoadLevelPacket::SCLoadLevelPacket(const GameParams &params) : mParams(params) {
}

// NTSC-U/C: 0x003e5040, PAL: 0x0041d2d8
Message *SCLoadLevelPacket::New() {
    return new SCLoadLevelPacket;
}

// NTSC-U/C: 0x003f13a0, PAL: 0x00429868
// Clone allocates and hands off to the copy constructor at 0x003f3c48, which is
// the compiler expanding the implicit one.
Message *SCLoadLevelPacket::Clone() {
    return new SCLoadLevelPacket(*this);
}

// NTSC-U/C: 0x003f1418, PAL: 0x004298e0
int SCLoadLevelPacket::Type() {
    return g_nSCLoadLevelPacketType;
}

// NTSC-U/C: 0x003f1428, PAL: 0x004298f0
const char *SCLoadLevelPacket::GetName() const {
    return "SCLoadLevelPacket";
}

// NTSC-U/C: 0x003f2858, PAL: 0x0042ada0
void SCLoadLevelPacket::PrintExtra(std::ostream &stream) const {
    mParams.Print(stream);
}

// NTSC-U/C: 0x003e8180, PAL: 0x00420460
void SCLoadLevelPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mParams.Save(&stream);
}

// NTSC-U/C: 0x003f27a0, PAL: 0x0042ace8
void SCLoadLevelPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mParams.Load(&stream);
}

// NTSC-U/C: 0x003f1528, PAL: 0x004299f0
GameParams SCLoadLevelPacket::GetParams() {
    return mParams;
}
