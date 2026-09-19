#include "msg/packet.h"

// 0x003f1de8
void Packet::Save(OBStream &stream) {
    int unknown04 = mUnknown04;
    stream.Write(&unknown04, sizeof(unknown04));

    int unknown08 = mUnknown08;
    stream.Write(&unknown08, sizeof(unknown08));

    int unknown0c = mUnknown0c;
    stream.Write(&unknown0c, sizeof(unknown0c));

    int unknown10 = mUnknown10;
    stream.Write(&unknown10, sizeof(unknown10));
}

// 0x003f1ea0
void Packet::Load(IBStream &stream) {
    stream.Read(&mUnknown04, sizeof(mUnknown04));
    stream.Read(&mUnknown08, sizeof(mUnknown08));
    stream.Read(&mUnknown0c, sizeof(mUnknown0c));
    stream.Read(&mUnknown10, sizeof(mUnknown10));
}
