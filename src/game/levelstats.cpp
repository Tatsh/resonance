#include "game/levelstats.h"

namespace {

constexpr char kRecordVersion = 2;

constexpr int kFirstRecordVersion = 1;
constexpr int kSecondRecordVersion = 2;

// The skill records Reset() appends, one per difficulty.
constexpr int kSkillCount = 3;

// The name Reset() assigns.
const char *const kEmptyName = "";

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

    char nStage = mStage;
    stream.WriteBytes(&nStage, sizeof(nStage));

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

        stream.Read(&mStage, sizeof(mStage));

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

        char nStage;
        stream.ReadBytes(&nStage, sizeof(nStage));
        mStage = nStage;

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

// 0x00142610
void LevelStats::Reset() {
    mName = kEmptyName;
    mStage = 0;
    mSkills.clear();
    for (int i = 0; i < kSkillCount; ++i) {
        SkillStats skill;
        skill.Clear();
        mSkills.push_back(skill);
    }
}

// 0x00142d70
void LevelStats::Assign(const LevelStats &other) {
    mName = other.mName;
    mStage = other.mStage;
    mSkills.clear();
    mSkills.resize(other.mSkills.size(), SkillStats());
    for (unsigned i = 0; i < mSkills.size(); ++i) {
        mSkills[i] = other.mSkills[i];
    }
}

// 0x001452f0
void LevelStats::Print([[maybe_unused]] std::ostream &stream) const {
}
