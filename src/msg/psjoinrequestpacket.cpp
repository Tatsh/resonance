#include "msg/psjoinrequestpacket.h"

#include <iostream>

// 0x003eef50
PSJoinRequestPacket::PSJoinRequestPacket() {
}

// 0x003e4a98
Message *PSJoinRequestPacket::New() {
    return new PSJoinRequestPacket;
}

// 0x003f1f38
void PSJoinRequestPacket::Print(std::ostream &stream) {
    mAppearance.Print(stream);
}

// 0x003eeeb8
// Clone allocates and hands off to the copy constructor at 0x003f2dc0, which is
// the compiler expanding the implicit one.
Message *PSJoinRequestPacket::Clone() {
    return new PSJoinRequestPacket(*this);
}

// 0x003eef30
int PSJoinRequestPacket::Type() {
    return g_nPSJoinRequestPacketType;
}

// 0x003eef40
const char *PSJoinRequestPacket::Name() {
    return "PSJoinRequestPacket";
}

// 0x003e5538
void PSJoinRequestPacket::Save(OBStream &stream) {
    int destination = mDestination;
    stream.Write(&destination, sizeof(destination));

    int destinationSystem = mDestinationSystem;
    stream.Write(&destinationSystem, sizeof(destinationSystem));

    int clientId = mClientId;
    stream.Write(&clientId, sizeof(clientId));

    int targetClientId = mTargetClientId;
    stream.Write(&targetClientId, sizeof(targetClientId));

    mAppearance.Save(stream);

    // Yes, the binary writes the client identifier a second time.
    int clientIdAgain = mClientId;
    stream.Write(&clientIdAgain, sizeof(clientIdAgain));
}

// 0x003e5638
void PSJoinRequestPacket::Load(IBStream &stream) {
    stream.Read(&mDestination, sizeof(mDestination));
    stream.Read(&mDestinationSystem, sizeof(mDestinationSystem));
    stream.Read(&mClientId, sizeof(mClientId));
    stream.Read(&mTargetClientId, sizeof(mTargetClientId));

    mAppearance.Load(stream);

    // Yes, the binary reads the client identifier a second time.
    stream.Read(&mClientId, sizeof(mClientId));
}
