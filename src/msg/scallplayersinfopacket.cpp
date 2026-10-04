#include "msg/scallplayersinfopacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *SCAllPlayersInfoPacket::New() {
    return new SCAllPlayersInfoPacket;
}

Message *SCAllPlayersInfoPacket::Clone() {
    // The copy constructor at 0x003f34e8 is the compiler expanding the implicit one.
    return new SCAllPlayersInfoPacket(*this);
}

int SCAllPlayersInfoPacket::Type() {
    return g_nSCAllPlayersInfoPacketType;
}

const char *SCAllPlayersInfoPacket::GetName() const {
    return "SCAllPlayersInfoPacket";
}

void SCAllPlayersInfoPacket::PrintExtra(std::ostream &stream) const {
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

void SCAllPlayersInfoPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int count = static_cast<int>(mPlayers.size());
    stream.WriteLE(&count, sizeof(count));
    for (const auto &entry : mPlayers) {
        int pid = entry.mPid;
        int clientId = entry.mClientId;
        OBStream &rest = stream.WriteLE(&pid, sizeof(pid)).WriteLE(&clientId, sizeof(clientId));

        int trackCount = static_cast<int>(entry.mTracks.size());
        rest.WriteLE(&trackCount, sizeof(trackCount));
        for (int track : entry.mTracks) {
            rest.WriteLE(&track, sizeof(track));
        }
    }
}

void SCAllPlayersInfoPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    int count;
    stream.ReadLE(&count, sizeof(count));
    mPlayers.resize(count);
    for (auto &entry : mPlayers) {
        IBStream &rest = stream.ReadLE(&entry.mPid, sizeof(entry.mPid))
                             .ReadLE(&entry.mClientId, sizeof(entry.mClientId));

        int trackCount;
        rest.ReadLE(&trackCount, sizeof(trackCount));
        entry.mTracks.resize(trackCount);
        for (int &track : entry.mTracks) {
            rest.ReadLE(&track, sizeof(track));
        }
    }
}

void SCAllPlayersInfoPacket::AddPlayerInfo(const PlayerInfoEntry &entry) {
    mPlayers.push_back(entry);
}
