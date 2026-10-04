#include "msg/scloadlevelpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

SCLoadLevelPacket::SCLoadLevelPacket() {
}

SCLoadLevelPacket::SCLoadLevelPacket(const GameParams &params) : mParams(params) {
}

Message *SCLoadLevelPacket::New() {
    return new SCLoadLevelPacket;
}

Message *SCLoadLevelPacket::Clone() {
    // The copy constructor at 0x003f3c48 is the compiler expanding the implicit one.
    return new SCLoadLevelPacket(*this);
}

int SCLoadLevelPacket::Type() {
    return g_nSCLoadLevelPacketType;
}

const char *SCLoadLevelPacket::GetName() const {
    return "SCLoadLevelPacket";
}

void SCLoadLevelPacket::PrintExtra(std::ostream &stream) const {
    mParams.Print(stream);
}

void SCLoadLevelPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mParams.Save(&stream);
}

void SCLoadLevelPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mParams.Load(&stream);
}

GameParams SCLoadLevelPacket::GetParams() {
    return mParams;
}
