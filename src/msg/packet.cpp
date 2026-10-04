#include "msg/packet.h"

#include "stream/ibstream.h"
#include "stream/obstream.h"

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

void Packet::restoreGuts(IBStream &stream) {
    stream.ReadLE(&mDestination, sizeof(mDestination));
    stream.ReadLE(&mDestinationSystem, sizeof(mDestinationSystem));
    stream.ReadLE(&mClientId, sizeof(mClientId));
    stream.ReadLE(&mTargetClientId, sizeof(mTargetClientId));
}
