#include "msg/bsloadlevelpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

BSLoadLevelPacket::BSLoadLevelPacket() {
}

BSLoadLevelPacket::BSLoadLevelPacket(const GameParams &params) : mParams(params) {
    mClientId = 0;
}

Message *BSLoadLevelPacket::New() {
    return new BSLoadLevelPacket;
}

Message *BSLoadLevelPacket::Clone() {
    // The copy constructor at 0x003f3bc0 is the compiler expanding the implicit one.
    return new BSLoadLevelPacket(*this);
}

int BSLoadLevelPacket::Type() {
    return g_nBSLoadLevelPacketType;
}

const char *BSLoadLevelPacket::GetName() const {
    return "BSLoadLevelPacket";
}

void BSLoadLevelPacket::PrintExtra(std::ostream &stream) const {
    mParams.Print(stream);
}

void BSLoadLevelPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mParams.Save(&stream);

    int clientId = mClientId;
    stream.WriteLE(&clientId, sizeof(clientId));
}

void BSLoadLevelPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mParams.Load(&stream);
    stream.ReadLE(&mClientId, sizeof(mClientId));
}

GameParams BSLoadLevelPacket::GetParams() {
    return mParams;
}
