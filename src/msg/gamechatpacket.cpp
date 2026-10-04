#include "msg/gamechatpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *GameChatPacket::New() {
    return new GameChatPacket;
}

Message *GameChatPacket::Clone() {
    // The copy constructor at 0x003f3d88 is the compiler expanding the implicit one.
    return new GameChatPacket(*this);
}

int GameChatPacket::Type() {
    return g_nGameChatPacketType;
}

const char *GameChatPacket::GetName() const {
    return "GameChatPacket";
}

void GameChatPacket::PrintExtra(std::ostream &stream) const {
    stream << mSender << mText;
}

void GameChatPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    SaveHxStr(SaveHxStr(stream, mSender), mText);
}

void GameChatPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    LoadHxStr(LoadHxStr(stream, mSender), mText);
}

GameChatPacket::GameChatPacket(const HxStr &sender, const HxStr &text)
    : mSender(sender), mText(text) {
}

HxStr GameChatPacket::GetSender() {
    return mSender;
}

HxStr GameChatPacket::GetText() {
    return mText;
}
