#include "game/skillstats.h"

namespace {

// Written by Save() and consulted by nothing, because Load() branches on the campaign-wide
// version instead.
constexpr char kRecordVersion = 2;

constexpr int kFirstRecordVersion = 1;
constexpr int kSecondRecordVersion = 2;

} // namespace

int g_nStatsRecordVersion;

// 0x001452f8
SkillStats::~SkillStats() {
}

// 0x00145338
void SkillStats::Save(OBStream &stream) {
    char nVersion = kRecordVersion;
    stream.WriteBytes(&nVersion, sizeof(nVersion));

    char bUnknown00 = mUnknown00 != 0;
    stream.WriteBytes(&bUnknown00, sizeof(bUnknown00));

    short nUnknown04 = mUnknown04;
    stream.Write(&nUnknown04, sizeof(nUnknown04));
}

// 0x00142f88
void SkillStats::Load(IBStream &stream) {
    if (g_nStatsRecordVersion == kFirstRecordVersion) {
        int nUnused;
        stream.Read(&nUnused, sizeof(nUnused));
        stream >> mUnknown00;
        stream.Read(&mUnknown04, sizeof(mUnknown04));
    } else if (g_nStatsRecordVersion == kSecondRecordVersion) {
        char nVersion;
        stream.ReadBytes(&nVersion, sizeof(nVersion));

        char bUnknown00;
        stream.ReadBytes(&bUnknown00, sizeof(bUnknown00));
        mUnknown00 = bUnknown00 != 0;

        unsigned short nUnknown04;
        stream.Read(&nUnknown04, sizeof(nUnknown04));
        mUnknown04 = nUnknown04;
    }
}
