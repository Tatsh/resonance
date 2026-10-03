#include "game/skillstats.h"

namespace {

// Written by Save() and consulted by nothing, because Load() branches on the campaign-wide
// version instead.
constexpr char kRecordVersion = 2;

constexpr int kFirstRecordVersion = 1;
constexpr int kSecondRecordVersion = 2;

} // namespace

int g_nStatsRecordVersion;

// NTSC-U/C: 0x001452f8, PAL: 0x00145e10
SkillStats::~SkillStats() {
}

// NTSC-U/C: 0x00145338, PAL: 0x00145e50
void SkillStats::Save(OBStream &stream) {
    char nVersion = kRecordVersion;
    stream.WriteBytes(&nVersion, sizeof(nVersion));

    char bBeaten = mBeaten != 0;
    stream.WriteBytes(&bBeaten, sizeof(bBeaten));

    short nHighScore = mHighScore;
    stream.Write(&nHighScore, sizeof(nHighScore));
}

// NTSC-U/C: 0x00142f88, PAL: 0x00143a88
void SkillStats::Load(IBStream &stream) {
    if (g_nStatsRecordVersion == kFirstRecordVersion) {
        int nUnused;
        stream.Read(&nUnused, sizeof(nUnused));
        stream >> mBeaten;
        stream.Read(&mHighScore, sizeof(mHighScore));
    } else if (g_nStatsRecordVersion == kSecondRecordVersion) {
        char nVersion;
        stream.ReadBytes(&nVersion, sizeof(nVersion));

        char bBeaten;
        stream.ReadBytes(&bBeaten, sizeof(bBeaten));
        mBeaten = bBeaten != 0;

        unsigned short nHighScore;
        stream.Read(&nHighScore, sizeof(nHighScore));
        mHighScore = nHighScore;
    }
}

// NTSC-U/C: 0x00145328, PAL: 0x00145e40
void SkillStats::Clear() {
    mHighScore = 0;
    mBeaten = 0;
}
