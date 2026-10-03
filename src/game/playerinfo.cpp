#include "game/playerinfo.h"

#include <iostream>

// NTSC-U/C: 0x00135e98, PAL: 0x00136750
PlayerInfo::~PlayerInfo() {
}

// NTSC-U/C: 0x00133930, PAL: 0x00134198
void PlayerInfo::Print(std::ostream &stream) {
    stream << "{AppPlayerInfo: " << mPlayerId << " " << mColorName << " ";
    mAppearance.Print(stream);
    stream << " " << mTrack << " " << mActive << " " << mReady << " ";
    stream << "(";
    for (std::vector<int>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        stream << *it << " ";
    }
    stream << ")";
    stream << "}";
}

// NTSC-U/C: 0x00133578, PAL: 0x00133de0
void PlayerInfo::Save(OBStream &stream) {
    unsigned playerId = mPlayerId;
    stream.Write(&playerId, sizeof(playerId));

    unsigned length = mColorName.mLen;
    stream.Write(&length, sizeof(length));
    // An empty name has no buffer, and the stream receives the shared empty string in place of a
    // null pointer.
    stream.WriteBytes(mColorName.mStr != nullptr ? mColorName.mStr : g_szEmptyString, length);

    mAppearance.Save(stream);

    int track = mTrack;
    stream.Write(&track, sizeof(track));

    int active = mActive;
    stream.Write(&active, sizeof(active));

    int ready = mReady;
    stream.Write(&ready, sizeof(ready));

    int count = mEntries.end() - mEntries.begin();
    stream.Write(&count, sizeof(count));
    for (std::vector<int>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        int element = *it;
        stream.Write(&element, sizeof(element));
    }
}

// NTSC-U/C: 0x00133740, PAL: 0x00133fa8
void PlayerInfo::Load(IBStream &stream) {
    stream.Read(&mPlayerId, sizeof(mPlayerId));

    unsigned length;
    stream.Read(&length, sizeof(length));
    mColorName.Alloc(length);
    stream.ReadBytes(
        mColorName.mStr != nullptr ? mColorName.mStr : const_cast<char *>(g_szEmptyString), length);

    mAppearance.Load(stream);

    stream.Read(&mTrack, sizeof(mTrack));
    stream.Read(&mActive, sizeof(mActive));
    stream.Read(&mReady, sizeof(mReady));

    int count;
    stream.Read(&count, sizeof(count));
    mEntries.resize(count);
    for (std::vector<int>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        stream.Read(&*it, sizeof(*it));
    }
}
