#include "msg/testarbiterpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

Message *TestArbiterPacket::New() {
    return new TestArbiterPacket;
}

Message *TestArbiterPacket::Clone() {
    // The copy constructor at 0x003f3e58 is the compiler expanding the implicit one.
    return new TestArbiterPacket(*this);
}

int TestArbiterPacket::Type() {
    return g_nTestArbiterPacketType;
}

const char *TestArbiterPacket::GetName() const {
    return "TestArbiterPacket";
}

void TestArbiterPacket::PrintExtra(std::ostream &stream) const {
    stream << mSender << mText;
}

void TestArbiterPacket::saveGuts(OBStream &stream) const {
    Packet::saveGuts(stream);
    SaveHxStr(SaveHxStr(stream, mSender), mText);
}

void TestArbiterPacket::restoreGuts(IBStream &stream) {
    Packet::restoreGuts(stream);
    LoadHxStr(LoadHxStr(stream, mSender), mText);
}

TestArbiterPacket::TestArbiterPacket(const HxStr &sender, const HxStr &text)
    : mSender(sender), mText(text) {
}

HxStr TestArbiterPacket::GetSender() {
    return mSender;
}

HxStr TestArbiterPacket::GetText() {
    return mText;
}
