#include "msg/testarbiterpacket.h"

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
