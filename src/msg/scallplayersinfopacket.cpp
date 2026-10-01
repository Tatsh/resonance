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
    for (const auto &entry : mPlayers) {
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

    int count = static_cast<int>(mPlayers.size());
    stream.Write(&count, sizeof(count));
    for (const auto &entry : mPlayers) {
        int pid = entry.mPid;
        int clientId = entry.mClientId;
        OBStream &rest = stream.Write(&pid, sizeof(pid)).Write(&clientId, sizeof(clientId));

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
    mPlayers.resize(count);
    for (auto &entry : mPlayers) {
        IBStream &rest = stream.Read(&entry.mPid, sizeof(entry.mPid))
                             .Read(&entry.mClientId, sizeof(entry.mClientId));

        int trackCount;
        rest.Read(&trackCount, sizeof(trackCount));
        entry.mTracks.resize(trackCount);
        for (int &track : entry.mTracks) {
            rest.Read(&track, sizeof(track));
        }
    }
}

// 0x003f23a8
void SCAllPlayersInfoPacket::AppendEntry(const PlayerEntry &entry) {
    mPlayers.push_back(entry);
}
