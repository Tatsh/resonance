#include "msg/bsloadlevelpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003f11b0
BSLoadLevelPacket::BSLoadLevelPacket() {
}

// 0x003f1220
BSLoadLevelPacket::BSLoadLevelPacket(const GameParams &params) : mUnknown14(params) {
    mUnknown0c = 0;
}

// 0x003e4f98
Message *BSLoadLevelPacket::New() {
    return new BSLoadLevelPacket;
}

// 0x003f1118. Clone allocates and hands off to the copy constructor at 0x003f3bc0, which is
// the compiler expanding the implicit one.
Message *BSLoadLevelPacket::Clone() {
    return new BSLoadLevelPacket(*this);
}

// 0x003f1190
int BSLoadLevelPacket::Type() {
    return g_nBSLoadLevelPacketType;
}

// 0x003f11a0
const char *BSLoadLevelPacket::Name() {
    return "BSLoadLevelPacket";
}

// 0x003f2780
void BSLoadLevelPacket::Print(std::ostream &stream) {
    mUnknown14.Print(stream);
}

// 0x003e7fa0. The word at +0x0c crosses the wire twice.
void BSLoadLevelPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    mUnknown14.Save(&stream);

    int unknown0c = mUnknown0c;
    stream.Write(&unknown0c, sizeof(unknown0c));
}

// 0x003e80a0
void BSLoadLevelPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    mUnknown14.Load(&stream);
    stream.Read(&mUnknown0c, sizeof(mUnknown0c));
}
