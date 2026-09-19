#include "msg/psjoinrequestpacket.h"

// 0x003eeeb8. Clone allocates and hands off to the copy constructor at 0x003f2dc0, which is
// the compiler expanding the implicit one.
Message *PSJoinRequestPacket::Clone() {
    return new PSJoinRequestPacket(*this);
}

// 0x003eef30
int PSJoinRequestPacket::Type() {
    return g_nPSJoinRequestPacketType;
}

// 0x003eef40
const char *PSJoinRequestPacket::Name() {
    return "PSJoinRequestPacket";
}

// 0x003e5538
void PSJoinRequestPacket::Save(OBStream &stream) {
    int unknown04 = mUnknown04;
    stream.Write(&unknown04, sizeof(unknown04));

    int unknown08 = mUnknown08;
    stream.Write(&unknown08, sizeof(unknown08));

    int unknown0c = mUnknown0c;
    stream.Write(&unknown0c, sizeof(unknown0c));

    int unknown10 = mUnknown10;
    stream.Write(&unknown10, sizeof(unknown10));

    mUnknown14.Save(stream);

    // Yes, the binary writes the word at +0x0c a second time.
    int unknown0cAgain = mUnknown0c;
    stream.Write(&unknown0cAgain, sizeof(unknown0cAgain));
}

// 0x003e5638
void PSJoinRequestPacket::Load(IBStream &stream) {
    stream.Read(&mUnknown04, sizeof(mUnknown04));
    stream.Read(&mUnknown08, sizeof(mUnknown08));
    stream.Read(&mUnknown0c, sizeof(mUnknown0c));
    stream.Read(&mUnknown10, sizeof(mUnknown10));

    mUnknown14.Load(stream);

    // Yes, the binary reads the word at +0x0c a second time.
    stream.Read(&mUnknown0c, sizeof(mUnknown0c));
}
