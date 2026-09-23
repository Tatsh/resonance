#include "msg/cripplepacket.h"

#include <iostream>

#include "game/player.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e7668
CripplePacket::CripplePacket(Player *pAttacker, std::vector<Player *> victims)
    : mAttacker(pAttacker) {
    for (std::vector<Player *>::iterator it = victims.begin(); it != victims.end(); ++it) {
        mTargets.push_back(IDablePtr<Player>(*it));
    }
}

// 0x003e53a0
Message *CripplePacket::New() {
    return new CripplePacket;
}

// 0x003f0c60
// Clone allocates and hands off to the copy constructor at 0x003f3938, which is
// the compiler expanding the implicit one.
Message *CripplePacket::Clone() {
    return new CripplePacket(*this);
}

// 0x003f0cd8
int CripplePacket::Type() {
    return g_nCripplePacketType;
}

// 0x003f0ce8
const char *CripplePacket::Name() {
    return "CripplePacket";
}

// 0x003e7c38
void CripplePacket::Print(std::ostream &stream) {
    std::ostream &rest = stream << static_cast<void *>(static_cast<Player *>(mAttacker)) << " ";
    rest << "(";
    for (auto &player : mTargets) {
        rest << static_cast<void *>(static_cast<Player *>(player)) << " ";
    }
    rest << ")";
}

// 0x003e7938
void CripplePacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int id = mAttacker.mId;
    OBStream &rest = stream.Write(&id, sizeof(id));

    int count = static_cast<int>(mTargets.size());
    rest.Write(&count, sizeof(count));
    for (const auto &player : mTargets) {
        int playerId = player.mId;
        rest.Write(&playerId, sizeof(playerId));
    }
}

// 0x003e7aa0
void CripplePacket::Load(IBStream &stream) {
    Packet::Load(stream);

    IBStream &rest = stream.Read(&mAttacker.mId, sizeof(mAttacker.mId));

    int count;
    rest.Read(&count, sizeof(count));
    mTargets.resize(count);
    for (auto &player : mTargets) {
        rest.Read(&player.mId, sizeof(player.mId));
    }
}
