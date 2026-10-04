#include "msg/cripplepacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

CripplePacket::CripplePacket(Player *pAttacker, std::vector<Player *> victims)
    : mAttacker(pAttacker) {
    for (std::vector<Player *>::iterator it = victims.begin(); it != victims.end(); ++it) {
        mTargets.push_back(IDablePtr<Player>(*it));
    }
}

Message *CripplePacket::New() {
    return new CripplePacket;
}

Message *CripplePacket::Clone() {
    // The copy constructor at 0x003f3938 is the compiler expanding the implicit one.
    return new CripplePacket(*this);
}

int CripplePacket::Type() {
    return g_nCripplePacketType;
}

const char *CripplePacket::GetName() const {
    return "CripplePacket";
}

void CripplePacket::PrintExtra(std::ostream &stream) const {
    std::ostream &rest = stream << static_cast<void *>(static_cast<Player *>(mAttacker)) << " ";
    rest << "(";
    for (auto &player : mTargets) {
        rest << static_cast<void *>(static_cast<Player *>(player)) << " ";
    }
    rest << ")";
}

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
