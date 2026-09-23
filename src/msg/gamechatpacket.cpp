#include "msg/gamechatpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e5478
Message *GameChatPacket::New() {
    return new GameChatPacket;
}

// 0x003f1950. Clone allocates and hands off to the copy constructor at 0x003f3d88, which is
// the compiler expanding the implicit one.
Message *GameChatPacket::Clone() {
    return new GameChatPacket(*this);
}

// 0x003f19c8
int GameChatPacket::Type() {
    return g_nGameChatPacketType;
}

// 0x003f19d8
const char *GameChatPacket::Name() {
    return "GameChatPacket";
}

// 0x003f28e8
void GameChatPacket::Print(std::ostream &stream) {
    stream << mUnknown14 << mUnknown1c;
}

// 0x003e8458
void GameChatPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    SaveHxStr(SaveHxStr(stream, mUnknown14), mUnknown1c);
}

// 0x003e85c8
void GameChatPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    LoadHxStr(LoadHxStr(stream, mUnknown14), mUnknown1c);
}
