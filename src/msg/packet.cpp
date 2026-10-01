#include "msg/packet.h"

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003f1de8
void Packet::Save(OBStream &stream) {
    int destination = mDestination;
    stream.Write(&destination, sizeof(destination));

    int destinationSystem = mDestinationSystem;
    stream.Write(&destinationSystem, sizeof(destinationSystem));

    int clientId = mClientId;
    stream.Write(&clientId, sizeof(clientId));

    int targetClientId = mTargetClientId;
    stream.Write(&targetClientId, sizeof(targetClientId));
}

// 0x003f1ea0
void Packet::Load(IBStream &stream) {
    stream.Read(&mDestination, sizeof(mDestination));
    stream.Read(&mDestinationSystem, sizeof(mDestinationSystem));
    stream.Read(&mClientId, sizeof(mClientId));
    stream.Read(&mTargetClientId, sizeof(mTargetClientId));
}
