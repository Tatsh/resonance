#include "msg/cripplepacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e7668, PAL: 0x0041f948
CripplePacket::CripplePacket(Player *pAttacker, std::vector<Player *> victims)
    : mAttacker(pAttacker) {
    for (std::vector<Player *>::iterator it = victims.begin(); it != victims.end(); ++it) {
        mTargets.push_back(IDablePtr<Player>(*it));
    }
}

// NTSC-U/C: 0x003e53a0, PAL: 0x0041d638
Message *CripplePacket::New() {
    return new CripplePacket;
}

// NTSC-U/C: 0x003f0c60, PAL: 0x00429268
// Clone allocates and hands off to the copy constructor at 0x003f3938, which is
// the compiler expanding the implicit one.
Message *CripplePacket::Clone() {
    return new CripplePacket(*this);
}

// NTSC-U/C: 0x003f0cd8, PAL: 0x004292e0
int CripplePacket::Type() {
    return g_nCripplePacketType;
}

// NTSC-U/C: 0x003f0ce8, PAL: 0x004292f0
const char *CripplePacket::GetName() const {
    return "CripplePacket";
}

// NTSC-U/C: 0x003e7c38, PAL: 0x0041ff18
void CripplePacket::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << static_cast<void *>(static_cast<Player *>(mAttacker)) << " ";
    rest << "(";
    for (auto &player : mTargets) {
        rest << static_cast<void *>(static_cast<Player *>(player)) << " ";
    }
    rest << ")";
}

// NTSC-U/C: 0x003e7938, PAL: 0x0041fc18
void CripplePacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);

    int id = mAttacker.mId;
    OBStream &rest = stream.WriteLE(&id, sizeof(id));

    int count = static_cast<int>(mTargets.size());
    rest.WriteLE(&count, sizeof(count));
    for (const auto &player : mTargets) {
        int playerId = player.mId;
        rest.WriteLE(&playerId, sizeof(playerId));
    }
}

// NTSC-U/C: 0x003e7aa0, PAL: 0x0041fd80
void CripplePacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);

    IBStream &rest = stream.ReadLE(&mAttacker.mId, sizeof(mAttacker.mId));

    int count;
    rest.ReadLE(&count, sizeof(count));
    mTargets.resize(count);
    for (auto &player : mTargets) {
        rest.ReadLE(&player.mId, sizeof(player.mId));
    }
}
