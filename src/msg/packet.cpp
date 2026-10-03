#include "msg/packet.h"

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003f1de8, PAL: 0x0042a330
void Packet::saveGuts(OBStream &stream) const {
    int destination = mDestination;
    stream.Write(&destination, sizeof(destination));

    int destinationSystem = mDestinationSystem;
    stream.Write(&destinationSystem, sizeof(destinationSystem));

    int clientId = mClientId;
    stream.Write(&clientId, sizeof(clientId));

    int targetClientId = mTargetClientId;
    stream.Write(&targetClientId, sizeof(targetClientId));
}

// NTSC-U/C: 0x003f1ea0, PAL: 0x0042a3e8
void Packet::restoreGuts(IBStream &stream) {
    stream.Read(&mDestination, sizeof(mDestination));
    stream.Read(&mDestinationSystem, sizeof(mDestinationSystem));
    stream.Read(&mClientId, sizeof(mClientId));
    stream.Read(&mTargetClientId, sizeof(mTargetClientId));
}
