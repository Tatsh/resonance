#include "msg/psjoinrequestpacket.h"

#include <iostream>

PSJoinRequestPacket::PSJoinRequestPacket() {
}

Message *PSJoinRequestPacket::New() {
    return new PSJoinRequestPacket;
}

void PSJoinRequestPacket::PrintExtra(std::ostream &stream) const {
    mAppearance.Print(stream);
}

Message *PSJoinRequestPacket::Clone() {
    // The copy constructor at 0x003f2dc0 is the compiler expanding the implicit one.
    return new PSJoinRequestPacket(*this);
}

int PSJoinRequestPacket::Type() {
    return g_nPSJoinRequestPacketType;
}

const char *PSJoinRequestPacket::GetName() const {
    return "PSJoinRequestPacket";
}

void PSJoinRequestPacket::saveGuts(OBStream &stream) const {
    int destination = mDestination;
    stream.WriteLE(&destination, sizeof(destination));

    int destinationSystem = mDestinationSystem;
    stream.WriteLE(&destinationSystem, sizeof(destinationSystem));

    int clientId = mClientId;
    stream.WriteLE(&clientId, sizeof(clientId));

    int targetClientId = mTargetClientId;
    stream.WriteLE(&targetClientId, sizeof(targetClientId));

    mAppearance.Save(stream);

    // Yes, the binary writes the client identifier a second time.
    int clientIdAgain = mClientId;
    stream.WriteLE(&clientIdAgain, sizeof(clientIdAgain));
}

void PSJoinRequestPacket::restoreGuts(IBStream &stream) {
    stream.ReadLE(&mDestination, sizeof(mDestination));
    stream.ReadLE(&mDestinationSystem, sizeof(mDestinationSystem));
    stream.ReadLE(&mClientId, sizeof(mClientId));
    stream.ReadLE(&mTargetClientId, sizeof(mTargetClientId));

    mAppearance.Load(stream);

    // Yes, the binary reads the client identifier a second time.
    stream.ReadLE(&mClientId, sizeof(mClientId));
}
