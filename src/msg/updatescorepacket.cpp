#include "msg/updatescorepacket.h"

#include "stream/ibstream.h"
#include "stream/obstream.h"

// 0x003f06c0. Clone allocates and hands off to the copy constructor at 0x003f3818, which is
// the compiler expanding the implicit one.
Message *UpdateScorePacket::Clone() {
    return new UpdateScorePacket(*this);
}

// 0x003f0738
int UpdateScorePacket::Type() {
    return g_nUpdateScorePacketType;
}

// 0x003f0748
const char *UpdateScorePacket::Name() {
    return "UpdateScorePacket";
}

// 0x003f2510
void UpdateScorePacket::Print(ostream &stream) {
    stream << "pid:" << mUnknown14 << " score-delta:" << mUnknown18;
}

// 0x003e7018
void UpdateScorePacket::Save(OBStream &stream) {
    Packet::Save(stream);

    int unknown14 = mUnknown14;
    stream.Write(&unknown14, sizeof(unknown14));

    int unknown18 = mUnknown18;
    stream.Write(&unknown18, sizeof(unknown18));
}

// 0x003e7120
void UpdateScorePacket::Load(IBStream &stream) {
    Packet::Load(stream);
    stream.Read(&mUnknown14, sizeof(mUnknown14));
    stream.Read(&mUnknown18, sizeof(mUnknown18));
}
