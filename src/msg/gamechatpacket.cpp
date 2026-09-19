#include "msg/gamechatpacket.h"

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
