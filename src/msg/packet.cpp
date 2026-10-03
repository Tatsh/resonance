#include "msg/packet.h"

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003f1de8, PAL: 0x0042a330
void Packet::saveGuts(OBStream &stream) const {
    int destination = mDestination;
    stream.WriteLE(&destination, sizeof(destination));

    int destinationSystem = mDestinationSystem;
    stream.WriteLE(&destinationSystem, sizeof(destinationSystem));

    int clientId = mClientId;
    stream.WriteLE(&clientId, sizeof(clientId));

    int targetClientId = mTargetClientId;
    stream.WriteLE(&targetClientId, sizeof(targetClientId));
}

// NTSC-U/C: 0x003f1ea0, PAL: 0x0042a3e8
void Packet::restoreGuts(IBStream &stream) {
    stream.ReadLE(&mDestination, sizeof(mDestination));
    stream.ReadLE(&mDestinationSystem, sizeof(mDestinationSystem));
    stream.ReadLE(&mClientId, sizeof(mClientId));
    stream.ReadLE(&mTargetClientId, sizeof(mTargetClientId));
}
