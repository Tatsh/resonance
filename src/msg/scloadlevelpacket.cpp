#include "msg/scloadlevelpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003f1438
SCLoadLevelPacket::SCLoadLevelPacket() {
}

// 0x003f14b0
SCLoadLevelPacket::SCLoadLevelPacket(const GameParams &params) : mUnknown14(params) {
}

// 0x003e5040
Message *SCLoadLevelPacket::New() {
    return new SCLoadLevelPacket;
}

// 0x003f13a0. Clone allocates and hands off to the copy constructor at 0x003f3c48, which is
// the compiler expanding the implicit one.
Message *SCLoadLevelPacket::Clone() {
    return new SCLoadLevelPacket(*this);
}

// 0x003f1418
int SCLoadLevelPacket::Type() {
    return g_nSCLoadLevelPacketType;
}

// 0x003f1428
const char *SCLoadLevelPacket::Name() {
    return "SCLoadLevelPacket";
}

// 0x003f2858
void SCLoadLevelPacket::Print(std::ostream &stream) {
    mUnknown14.Print(stream);
}

// 0x003e8180
void SCLoadLevelPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    mUnknown14.Save(&stream);
}

// 0x003f27a0
void SCLoadLevelPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    mUnknown14.Load(&stream);
}
