#include "msg/gamechatpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e5478, PAL: 0x0041d710
Message *GameChatPacket::New() {
    return new GameChatPacket;
}

// NTSC-U/C: 0x003f1950, PAL: 0x00429e40
// Clone allocates and hands off to the copy constructor at 0x003f3d88, which is
// the compiler expanding the implicit one.
Message *GameChatPacket::Clone() {
    return new GameChatPacket(*this);
}

// NTSC-U/C: 0x003f19c8, PAL: 0x00429eb8
int GameChatPacket::Type() {
    return g_nGameChatPacketType;
}

// NTSC-U/C: 0x003f19d8, PAL: 0x00429ec8
const char *GameChatPacket::GetName() const {
    return "GameChatPacket";
}

// NTSC-U/C: 0x003f28e8, PAL: 0x0042ae30
void GameChatPacket::PrintExtra(std::ostream &stream) const {
    stream << mSender << mText;
}

// NTSC-U/C: 0x003e8458, PAL: 0x00420738
void GameChatPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    SaveHxStr(SaveHxStr(stream, mSender), mText);
}

// NTSC-U/C: 0x003e85c8, PAL: 0x004208a8
void GameChatPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    LoadHxStr(LoadHxStr(stream, mSender), mText);
}

// NTSC-U/C: 0x003f1a28, PAL: 0x00429f20
GameChatPacket::GameChatPacket(const HxStr &sender, const HxStr &text)
    : mSender(sender), mText(text) {
}

// NTSC-U/C: 0x003f1ae0, PAL: 0x00429fe8
HxStr GameChatPacket::GetSender() {
    return mSender;
}

// NTSC-U/C: 0x003f1b10, PAL: 0x0042a018
HxStr GameChatPacket::GetText() {
    return mText;
}
