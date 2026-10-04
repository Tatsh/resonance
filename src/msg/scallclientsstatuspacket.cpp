#include "msg/scallclientsstatuspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *SCAllClientsStatusPacket::New() {
    return new SCAllClientsStatusPacket;
}

Message *SCAllClientsStatusPacket::Clone() {
    // The copy constructor at 0x003f3298 is the compiler expanding the implicit one.
    return new SCAllClientsStatusPacket(*this);
}

int SCAllClientsStatusPacket::Type() {
    return g_nSCAllClientsStatusPacketType;
}

const char *SCAllClientsStatusPacket::GetName() const {
    return "SCAllClientsStatusPacket";
}

void SCAllClientsStatusPacket::PrintExtra(std::ostream &stream) const {
    for (const auto &entry : mClients) {
        stream << "(id:" << entry.mId << " stat:" << entry.mStatus << ") ";
    }
}

void SCAllClientsStatusPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int count = static_cast<int>(mClients.size());
    stream.WriteLE(&count, sizeof(count));
    for (const auto &entry : mClients) {
        int id = entry.mId;
        int status = entry.mStatus;
        stream.WriteLE(&id, sizeof(id)).WriteLE(&status, sizeof(status));
    }
}

void SCAllClientsStatusPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    int count;
    stream.ReadLE(&count, sizeof(count));
    mClients.resize(count);
    for (auto &entry : mClients) {
        stream.ReadLE(&entry.mId, sizeof(entry.mId)).ReadLE(&entry.mStatus, sizeof(entry.mStatus));
    }
}

void SCAllClientsStatusPacket::AddClientStatus(int nId, int nStatus) {
    ClientInfoEntry entry;
    entry.mId = nId;
    entry.mStatus = nStatus;
    mClients.push_back(entry);
}
