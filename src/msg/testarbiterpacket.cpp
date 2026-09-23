#include "msg/testarbiterpacket.h"

#include <iostream>

#include "msg/hxstrtransfer.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003e54d8
Message *TestArbiterPacket::New() {
    return new TestArbiterPacket;
}

// 0x003f1bf8. Clone allocates and hands off to the copy constructor at 0x003f3e58, which is
// the compiler expanding the implicit one.
Message *TestArbiterPacket::Clone() {
    return new TestArbiterPacket(*this);
}

// 0x003f1c70
int TestArbiterPacket::Type() {
    return g_nTestArbiterPacketType;
}

// 0x003f1c80
const char *TestArbiterPacket::Name() {
    return "TestArbiterPacket";
}

// 0x003f2ab8
void TestArbiterPacket::Print(std::ostream &stream) {
    stream << mUnknown14 << mUnknown1c;
}

// 0x003e8720
void TestArbiterPacket::Save(OBStream &stream) {
    Packet::Save(stream);
    SaveHxStr(SaveHxStr(stream, mUnknown14), mUnknown1c);
}

// 0x003e8890
void TestArbiterPacket::Load(IBStream &stream) {
    Packet::Load(stream);
    LoadHxStr(LoadHxStr(stream, mUnknown14), mUnknown1c);
}
