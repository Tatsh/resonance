#include "game/levelstats.h"

namespace {

constexpr char kRecordVersion = 2;

constexpr int kFirstRecordVersion = 1;
constexpr int kSecondRecordVersion = 2;

} // namespace

// 0x001448b8
LevelStats::LevelStats() {
}

// 0x001424e8
LevelStats::~LevelStats() {
}

// 0x00142760
void LevelStats::Save(OBStream &stream) {
    char nVersion = kRecordVersion;
    stream.WriteBytes(&nVersion, sizeof(nVersion));

    unsigned nLength = mName.mLen;
    stream.Write(&nLength, sizeof(nLength));
    // An empty name has no buffer, and the stream receives the shared empty string in place of a
    // null pointer.
    stream.WriteBytes(mName.mStr != nullptr ? mName.mStr : g_szEmptyString, mName.mLen);

    char bUnknown08 = mUnknown08;
    stream.WriteBytes(&bUnknown08, sizeof(bUnknown08));

    mSkills[0].Save(stream);
    mSkills[1].Save(stream);
    mSkills[2].Save(stream);
}

// 0x00142880
void LevelStats::Load(IBStream &stream) {
    if (g_nStatsRecordVersion == kFirstRecordVersion) {
        int nUnused;
        stream.Read(&nUnused, sizeof(nUnused));

        unsigned nLength;
        stream.Read(&nLength, sizeof(nLength));
        mName.Alloc(nLength);
        // A zero-length name still arrives at the stream as the shared empty string, and the
        // transfer is zero bytes wide, so the stream never writes through it.
        stream.ReadBytes(mName.mStr != nullptr ? mName.mStr : const_cast<char *>(g_szEmptyString),
                         nLength);

        stream.Read(&mUnknown08, sizeof(mUnknown08));

        int nSkillCount;
        stream.Read(&nSkillCount, sizeof(nSkillCount));
        mSkills.resize(nSkillCount);
        for (std::vector<SkillStats>::iterator it = mSkills.begin(); it != mSkills.end(); ++it) {
            it->Load(stream);
        }
    } else if (g_nStatsRecordVersion == kSecondRecordVersion) {
        char nVersion;
        stream.ReadBytes(&nVersion, sizeof(nVersion));

        unsigned nLength;
        stream.Read(&nLength, sizeof(nLength));
        mName.Alloc(nLength);
        stream.ReadBytes(mName.mStr != nullptr ? mName.mStr : const_cast<char *>(g_szEmptyString),
                         nLength);

        char bUnknown08;
        stream.ReadBytes(&bUnknown08, sizeof(bUnknown08));
        mUnknown08 = bUnknown08;

        mSkills.clear();

        // The three records go into three separate locals rather than through a loop, which the
        // destruction order at the end of the body establishes.
        SkillStats first;
        first.Load(stream);
        mSkills.push_back(first);

        SkillStats second;
        second.Load(stream);
        mSkills.push_back(second);

        SkillStats third;
        third.Load(stream);
        mSkills.push_back(third);
    }
}
