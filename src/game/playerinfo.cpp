#include "game/playerinfo.h"

#include <iostream>

PlayerInfo::~PlayerInfo() {
}

void PlayerInfo::Print(std::ostream &stream) const {
    stream << "{AppPlayerInfo: " << mPlayerId << " " << mColorName << " ";
    mAppearance.Print(stream);
    stream << " " << mTrack << " " << mActive << " " << mReady << " ";
    stream << "(";
    for (std::vector<int>::const_iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        stream << *it << " ";
    }
    stream << ")";
    stream << "}";
}

void PlayerInfo::Save(OBStream &stream) const {
    unsigned playerId = mPlayerId;
    stream.WriteLE(&playerId, sizeof(playerId));

    unsigned length = mColorName.mLen;
    stream.WriteLE(&length, sizeof(length));
    // An empty name has no buffer, and the stream receives the shared empty string in place of a
    // null pointer.
    stream.Write(mColorName.mStr != nullptr ? mColorName.mStr : g_szEmptyString, length);

    mAppearance.Save(stream);

    int track = mTrack;
    stream.WriteLE(&track, sizeof(track));

    int active = mActive;
    stream.WriteLE(&active, sizeof(active));

    int ready = mReady;
    stream.WriteLE(&ready, sizeof(ready));

    int count = mEntries.end() - mEntries.begin();
    stream.WriteLE(&count, sizeof(count));
    for (std::vector<int>::const_iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        int element = *it;
        stream.WriteLE(&element, sizeof(element));
    }
}

void PlayerInfo::Load(IBStream &stream) {
    stream.ReadLE(&mPlayerId, sizeof(mPlayerId));

    unsigned length;
    stream.ReadLE(&length, sizeof(length));
    mColorName.Alloc(length);
    stream.Read(mColorName.mStr != nullptr ? mColorName.mStr : const_cast<char *>(g_szEmptyString),
                length);

    mAppearance.Load(stream);

    stream.ReadLE(&mTrack, sizeof(mTrack));
    stream.ReadLE(&mActive, sizeof(mActive));
    stream.ReadLE(&mReady, sizeof(mReady));

    int count;
    stream.ReadLE(&count, sizeof(count));
    mEntries.resize(count);
    for (std::vector<int>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
        stream.ReadLE(&*it, sizeof(*it));
    }
}
