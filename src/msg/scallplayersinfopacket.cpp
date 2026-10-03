#include "msg/scallplayersinfopacket.h"

#include <iostream>

#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e4ee8, PAL: 0x0041d180
Message *SCAllPlayersInfoPacket::New() {
    return new SCAllPlayersInfoPacket;
}

// NTSC-U/C: 0x003eff60, PAL: 0x00428568
// Clone allocates and hands off to the copy constructor at 0x003f34e8, which is
// the compiler expanding the implicit one.
Message *SCAllPlayersInfoPacket::Clone() {
    return new SCAllPlayersInfoPacket(*this);
}

// NTSC-U/C: 0x003effd8, PAL: 0x004285e0
int SCAllPlayersInfoPacket::Type() {
    return g_nSCAllPlayersInfoPacketType;
}

// NTSC-U/C: 0x003effe8, PAL: 0x004285f0
const char *SCAllPlayersInfoPacket::GetName() const {
    return "SCAllPlayersInfoPacket";
}

// NTSC-U/C: 0x003f2278, PAL: 0x0042a7c0
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

// NTSC-U/C: 0x003e6578, PAL: 0x0041e858
void SCAllPlayersInfoPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

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

// NTSC-U/C: 0x003e6788, PAL: 0x0041ea68
void SCAllPlayersInfoPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

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

// NTSC-U/C: 0x003f23a8, PAL: 0x0042a8f0
void SCAllPlayersInfoPacket::AppendEntry(const PlayerEntry &entry) {
    mPlayers.push_back(entry);
}
