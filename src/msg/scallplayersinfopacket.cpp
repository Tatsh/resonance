#include "msg/scallplayersinfopacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e4ee8
Message *SCAllPlayersInfoPacket::New() {
    return new SCAllPlayersInfoPacket;
}

// 0x003eff60
// Clone allocates and hands off to the copy constructor at 0x003f34e8, which is
// the compiler expanding the implicit one.
Message *SCAllPlayersInfoPacket::Clone() {
    return new SCAllPlayersInfoPacket(*this);
}

// 0x003effd8
int SCAllPlayersInfoPacket::Type() {
    return g_nSCAllPlayersInfoPacketType;
}

// 0x003effe8
const char *SCAllPlayersInfoPacket::Name() {
    return "SCAllPlayersInfoPacket";
}

// 0x003f2278
void SCAllPlayersInfoPacket::Print(std::ostream &stream) {
    for (const auto &entry : mUnknown14) {
        std::ostream &rest = stream << " pid:" << entry.mPid << " tracks:";
        rest << "(";
        for (int track : entry.mTracks) {
            rest << track << " ";
        }
        rest << ")";
        rest << " | ";
    }
}

// 0x003e6578
void SCAllPlayersInfoPacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int count = static_cast<int>(mUnknown14.size());
    stream.Write(&count, sizeof(count));
    for (const auto &entry : mUnknown14) {
        int pid = entry.mPid;
        int unknown04 = entry.mUnknown04;
        OBStream &rest = stream.Write(&pid, sizeof(pid)).Write(&unknown04, sizeof(unknown04));

        int trackCount = static_cast<int>(entry.mTracks.size());
        rest.Write(&trackCount, sizeof(trackCount));
        for (int track : entry.mTracks) {
            rest.Write(&track, sizeof(track));
        }
    }
}

// 0x003e6788
void SCAllPlayersInfoPacket::Load(IBStream &stream) {
    Packet::Load(stream);

    int count;
    stream.Read(&count, sizeof(count));
    mUnknown14.resize(count);
    for (auto &entry : mUnknown14) {
        IBStream &rest = stream.Read(&entry.mPid, sizeof(entry.mPid))
                             .Read(&entry.mUnknown04, sizeof(entry.mUnknown04));

        int trackCount;
        rest.Read(&trackCount, sizeof(trackCount));
        entry.mTracks.resize(trackCount);
        for (int &track : entry.mTracks) {
            rest.Read(&track, sizeof(track));
        }
    }
}

// 0x003f23a8
void SCAllPlayersInfoPacket::AppendEntry(const Entry14 &entry) {
    mUnknown14.push_back(entry);
}
