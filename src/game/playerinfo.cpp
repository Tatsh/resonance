#include "game/playerinfo.h"

#include <iostream>

// 0x00135e98
PlayerInfo::~PlayerInfo() {
}

// 0x00133930
void PlayerInfo::Print(std::ostream &stream) {
    stream << "{AppPlayerInfo: " << mUnknown00 << " " << mUnknown04 << " ";
    mAppearance.Print(stream);
    stream << " " << mUnknown20 << " " << mUnknown24 << " " << mUnknown28 << " ";
    stream << "(";
    for (std::vector<int>::iterator it = mUnknown30.begin(); it != mUnknown30.end(); ++it) {
        stream << *it << " ";
    }
    stream << ")";
    stream << "}";
}

// 0x00133578
void PlayerInfo::Save(OBStream &stream) {
    unsigned unknown00 = mUnknown00;
    stream.Write(&unknown00, sizeof(unknown00));

    unsigned length = mUnknown04.mLen;
    stream.Write(&length, sizeof(length));
    // An empty name has no buffer, and the stream receives the shared empty string in place of a
    // null pointer.
    stream.WriteBytes(mUnknown04.mStr != nullptr ? mUnknown04.mStr : g_szEmptyString, length);

    mAppearance.Save(stream);

    int unknown20 = mUnknown20;
    stream.Write(&unknown20, sizeof(unknown20));

    int unknown24 = mUnknown24;
    stream.Write(&unknown24, sizeof(unknown24));

    int unknown28 = mUnknown28;
    stream.Write(&unknown28, sizeof(unknown28));

    int count = mUnknown30.end() - mUnknown30.begin();
    stream.Write(&count, sizeof(count));
    for (std::vector<int>::iterator it = mUnknown30.begin(); it != mUnknown30.end(); ++it) {
        int element = *it;
        stream.Write(&element, sizeof(element));
    }
}

// 0x00133740
void PlayerInfo::Load(IBStream &stream) {
    stream.Read(&mUnknown00, sizeof(mUnknown00));

    unsigned length;
    stream.Read(&length, sizeof(length));
    mUnknown04.Alloc(length);
    stream.ReadBytes(
        mUnknown04.mStr != nullptr ? mUnknown04.mStr : const_cast<char *>(g_szEmptyString), length);

    mAppearance.Load(stream);

    stream.Read(&mUnknown20, sizeof(mUnknown20));
    stream.Read(&mUnknown24, sizeof(mUnknown24));
    stream.Read(&mUnknown28, sizeof(mUnknown28));

    int count;
    stream.Read(&count, sizeof(count));
    mUnknown30.resize(count);
    for (std::vector<int>::iterator it = mUnknown30.begin(); it != mUnknown30.end(); ++it) {
        stream.Read(&*it, sizeof(*it));
    }
}
