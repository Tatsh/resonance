#include "msg/testarbiterpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// NTSC-U/C: 0x003e54d8, PAL: 0x0041d778
Message *TestArbiterPacket::New() {
    return new TestArbiterPacket;
}

// NTSC-U/C: 0x003f1bf8, PAL: 0x0042a128
// Clone allocates and hands off to the copy constructor at 0x003f3e58, which is
// the compiler expanding the implicit one.
Message *TestArbiterPacket::Clone() {
    return new TestArbiterPacket(*this);
}

// NTSC-U/C: 0x003f1c70, PAL: 0x0042a1a0
int TestArbiterPacket::Type() {
    return g_nTestArbiterPacketType;
}

// NTSC-U/C: 0x003f1c80, PAL: 0x0042a1b0
const char *TestArbiterPacket::Name() {
    return "TestArbiterPacket";
}

// NTSC-U/C: 0x003f2ab8, PAL: 0x0042b000
void TestArbiterPacket::Print(std::ostream &stream) {
    stream << mSender << mText;
}

// NTSC-U/C: 0x003e8720, PAL: 0x00420a00
void TestArbiterPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    SaveHxStr(SaveHxStr(stream, mSender), mText);
}

// NTSC-U/C: 0x003e8890, PAL: 0x00420b70
void TestArbiterPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    LoadHxStr(LoadHxStr(stream, mSender), mText);
}

// NTSC-U/C: 0x003f1cd0, PAL: 0x0042a208
TestArbiterPacket::TestArbiterPacket(const HxStr &sender, const HxStr &text)
    : mSender(sender), mText(text) {
}

// NTSC-U/C: 0x003f1d88, PAL: 0x0042a2d0
HxStr TestArbiterPacket::GetSender() {
    return mSender;
}

// NTSC-U/C: 0x003f1db8, PAL: 0x0042a300
HxStr TestArbiterPacket::GetText() {
    return mText;
}
