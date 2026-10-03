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
const char *SCAllClientsStatusPacket::Name() {
    return "SCAllClientsStatusPacket";
}

// NTSC-U/C: 0x003f2160, PAL: 0x0042a6a8
void SCAllClientsStatusPacket::Print(std::ostream &stream) {
    for (const auto &entry : mClients) {
        stream << "(id:" << entry.mId << " stat:" << entry.mStatus << ") ";
    }
}

// NTSC-U/C: 0x003e6288, PAL: 0x0041e568
void SCAllClientsStatusPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int count = static_cast<int>(mClients.size());
    stream.Write(&count, sizeof(count));
    for (const auto &entry : mClients) {
        int id = entry.mId;
        int status = entry.mStatus;
        stream.Write(&id, sizeof(id)).Write(&status, sizeof(status));
    }
}

// NTSC-U/C: 0x003e63f0, PAL: 0x0041e6d0
void SCAllClientsStatusPacket::Load(IBStream &stream) {
    Packet::Load(stream);

    int count;
    stream.Read(&count, sizeof(count));
    mClients.resize(count);
    for (auto &entry : mClients) {
        stream.Read(&entry.mId, sizeof(entry.mId)).Read(&entry.mStatus, sizeof(entry.mStatus));
    }
}

// NTSC-U/C: 0x003f2218, PAL: 0x0042a760
void SCAllClientsStatusPacket::AppendEntry(ClientStatus entry) {
    mClients.push_back(entry);
}
