#include "msg/scallclientsstatuspacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e4e48
Message *SCAllClientsStatusPacket::New() {
    return new SCAllClientsStatusPacket;
}

// 0x003efaa8
// Clone allocates and hands off to the copy constructor at 0x003f3298, which is
// the compiler expanding the implicit one.
Message *SCAllClientsStatusPacket::Clone() {
    return new SCAllClientsStatusPacket(*this);
}

// 0x003efb20
int SCAllClientsStatusPacket::Type() {
    return g_nSCAllClientsStatusPacketType;
}

// 0x003efb30
const char *SCAllClientsStatusPacket::Name() {
    return "SCAllClientsStatusPacket";
}

// 0x003f2160
void SCAllClientsStatusPacket::Print(std::ostream &stream) {
    for (const auto &entry : mUnknown14) {
        stream << "(id:" << entry.mId << " stat:" << entry.mStatus << ") ";
    }
}

// 0x003e6288
void SCAllClientsStatusPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int count = static_cast<int>(mUnknown14.size());
    stream.Write(&count, sizeof(count));
    for (const auto &entry : mUnknown14) {
        int id = entry.mId;
        int status = entry.mStatus;
        stream.Write(&id, sizeof(id)).Write(&status, sizeof(status));
    }
}

// 0x003e63f0
void SCAllClientsStatusPacket::Load(IBStream &stream) {
    Packet::Load(stream);

    int count;
    stream.Read(&count, sizeof(count));
    mUnknown14.resize(count);
    for (auto &entry : mUnknown14) {
        stream.Read(&entry.mId, sizeof(entry.mId)).Read(&entry.mStatus, sizeof(entry.mStatus));
    }
}

// 0x003f2218
void SCAllClientsStatusPacket::AppendEntry(Entry14 entry) {
    mUnknown14.push_back(entry);
}
