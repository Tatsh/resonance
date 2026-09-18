#include "msg/caughtphrasepacket.h"

// 0x003f04b8. Clone allocates and hands off to the copy constructor at 0x003f37b8, which is
// the compiler expanding the implicit one.
Message *CaughtPhrasePacket::Clone() {
    return new CaughtPhrasePacket(*this);
}

// 0x003f0530
int CaughtPhrasePacket::Type() {
    return g_nCaughtPhrasePacketType;
}

// 0x003f0540
const char *CaughtPhrasePacket::Name() {
    return "CaughtPhrasePacket";
}
