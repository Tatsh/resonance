#include "msg/scallclientsstatuspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e4e48, PAL: 0x0041d0e0
Message *SCAllClientsStatusPacket::New() {
    return new SCAllClientsStatusPacket;
}

// NTSC-U/C: 0x003efaa8, PAL: 0x004280b0
// Clone allocates and hands off to the copy constructor at 0x003f3298, which is
// the compiler expanding the implicit one.
Message *SCAllClientsStatusPacket::Clone() {
    return new SCAllClientsStatusPacket(*this);
}

// NTSC-U/C: 0x003efb20, PAL: 0x00428128
int SCAllClientsStatusPacket::Type() {
    return g_nSCAllClientsStatusPacketType;
}

// NTSC-U/C: 0x003efb30, PAL: 0x00428138
const char *SCAllClientsStatusPacket::GetName() const {
    return "SCAllClientsStatusPacket";
}

// NTSC-U/C: 0x003f2160, PAL: 0x0042a6a8
void SCAllClientsStatusPacket::PrintExtra(std::ostream &stream) const {
    for (const auto &entry : mClients) {
        stream << "(id:" << entry.mId << " stat:" << entry.mStatus << ") ";
    }
}

// NTSC-U/C: 0x003e6288, PAL: 0x0041e568
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

// NTSC-U/C: 0x003e63f0, PAL: 0x0041e6d0
void SCAllClientsStatusPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    int count;
    stream.ReadLE(&count, sizeof(count));
    mClients.resize(count);
    for (auto &entry : mClients) {
        stream.ReadLE(&entry.mId, sizeof(entry.mId)).ReadLE(&entry.mStatus, sizeof(entry.mStatus));
    }
}

// NTSC-U/C: 0x003f2218, PAL: 0x0042a760
void SCAllClientsStatusPacket::AddClientStatus(int nId, int nStatus) {
    ClientInfoEntry entry;
    entry.mId = nId;
    entry.mStatus = nStatus;
    mClients.push_back(entry);
}
