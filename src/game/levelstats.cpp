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

LevelStats::LevelStats() {
}

LevelStats::~LevelStats() {
}

void LevelStats::Save(OBStream &stream) {
    char nVersion = kRecordVersion;
    stream.Write(&nVersion, sizeof(nVersion));

    unsigned nLength = mName.mLen;
    stream.WriteLE(&nLength, sizeof(nLength));
    // An empty name has no buffer, and the stream receives the shared empty string in place of a
    // null pointer.
    stream.Write(mName.mStr != nullptr ? mName.mStr : g_szEmptyString, mName.mLen);

    char nStage = mStage;
    stream.Write(&nStage, sizeof(nStage));

    mSkills[0].Save(stream);
    mSkills[1].Save(stream);
    mSkills[2].Save(stream);
}

void LevelStats::Load(IBStream &stream) {
    if (g_nStatsRecordVersion == kFirstRecordVersion) {
        int nUnused;
        stream.ReadLE(&nUnused, sizeof(nUnused));

        unsigned nLength;
        stream.ReadLE(&nLength, sizeof(nLength));
        mName.Alloc(nLength);
        // A zero-length name still arrives at the stream as the shared empty string, and the
        // transfer is zero bytes wide, so the stream never writes through it.
        stream.Read(mName.mStr != nullptr ? mName.mStr : const_cast<char *>(g_szEmptyString),
                    nLength);

        stream.ReadLE(&mStage, sizeof(mStage));

        int nSkillCount;
        stream.ReadLE(&nSkillCount, sizeof(nSkillCount));
        mSkills.resize(nSkillCount);
        for (std::vector<SkillStats>::iterator it = mSkills.begin(); it != mSkills.end(); ++it) {
            it->Load(stream);
        }
    } else if (g_nStatsRecordVersion == kSecondRecordVersion) {
        char nVersion;
        stream.Read(&nVersion, sizeof(nVersion));

        unsigned nLength;
        stream.ReadLE(&nLength, sizeof(nLength));
        mName.Alloc(nLength);
        stream.Read(mName.mStr != nullptr ? mName.mStr : const_cast<char *>(g_szEmptyString),
                    nLength);

        unsigned char nStage;
        stream.Read(&nStage, sizeof(nStage));
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

void LevelStats::Assign(const LevelStats &other) {
    mName = other.mName;
    mStage = other.mStage;
    mSkills.clear();
    // Yes, the binary fills with a temporary whose members are never set. The loop below overwrites
    // every element.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    mSkills.resize(other.mSkills.size(), SkillStats());
#pragma GCC diagnostic pop
    for (unsigned i = 0; i < mSkills.size(); ++i) {
        mSkills[i] = other.mSkills[i];
    }
}

void LevelStats::Print([[maybe_unused]] std::ostream &stream) const {
}
