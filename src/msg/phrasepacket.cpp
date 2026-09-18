#include "msg/phrasepacket.h"

// 0x003f0320. Clone allocates and hands off to the copy constructor at 0x003f3760, which is
// the compiler expanding the implicit one.
Message *PhrasePacket::Clone() {
    return new PhrasePacket(*this);
}

// 0x003f0398
int PhrasePacket::Type() {
    return g_nPhrasePacketType;
}

// 0x003f03a8
const char *PhrasePacket::Name() {
    return "PhrasePacket";
}
