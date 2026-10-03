#include "msg/bsloadlevelpacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003f11b0, PAL: 0x00429718
BSLoadLevelPacket::BSLoadLevelPacket() {
}

// NTSC-U/C: 0x003f1220, PAL: 0x00429788
BSLoadLevelPacket::BSLoadLevelPacket(const GameParams &params) : mParams(params) {
    mClientId = 0;
}

// NTSC-U/C: 0x003e4f98, PAL: 0x0041d230
Message *BSLoadLevelPacket::New() {
    return new BSLoadLevelPacket;
}

// NTSC-U/C: 0x003f1118, PAL: 0x00429680
// Clone allocates and hands off to the copy constructor at 0x003f3bc0, which is
// the compiler expanding the implicit one.
Message *BSLoadLevelPacket::Clone() {
    return new BSLoadLevelPacket(*this);
}

// NTSC-U/C: 0x003f1190, PAL: 0x004296f8
int BSLoadLevelPacket::Type() {
    return g_nBSLoadLevelPacketType;
}

// NTSC-U/C: 0x003f11a0, PAL: 0x00429708
const char *BSLoadLevelPacket::GetName() const {
    return "BSLoadLevelPacket";
}

// NTSC-U/C: 0x003f2780, PAL: 0x0042acc8
void BSLoadLevelPacket::PrintExtra(std::ostream &stream) const {
    mParams.Print(stream);
}

// NTSC-U/C: 0x003e7fa0, PAL: 0x00420280
// The word at +0x0c crosses the wire twice.
void BSLoadLevelPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    mParams.Save(&stream);

    int clientId = mClientId;
    stream.WriteLE(&clientId, sizeof(clientId));
}

// NTSC-U/C: 0x003e80a0, PAL: 0x00420380
void BSLoadLevelPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    mParams.Load(&stream);
    stream.ReadLE(&mClientId, sizeof(mClientId));
}

// NTSC-U/C: 0x003f1290, PAL: 0x004297f8
GameParams BSLoadLevelPacket::GetParams() {
    return mParams;
}
