#include "met/metpersonadata.h"

#include "game/controllerconfig.h"
#include "met/gameoptions.h"
#include "os/mem.h"

namespace {

static const char *const kAllocationTag = "MetPersonaData";
static const char *const kInitialBirthday = "0/0/00, 12:00";
static const char *const kNoText = "";

static const char *const kStatsLabel = " CampaignStats=";
static const char *const kAppearanceLabel = " FreqAppearance=";
static const char *const kBirthdayLabel = " Birthday=";
static const char *const kPrefabLabel = " IsPrefab=";

// Save() writes version 2. Load() discards the legacy lists at version 0 and below and reads the
// birthday from version 2.
constexpr int kRecordVersion = 2;
constexpr int kLastLegacyVersion = 0;
constexpr int kBirthdayVersion = 2;

// The skill statuses UpdateSkillStatus() records, from none to the secret stage.
constexpr int kSkillStatusNone = 0;
constexpr int kSkillStatusEasy = 1;
constexpr int kSkillStatusNormal = 2;
constexpr int kSkillStatusExpert = 3;
constexpr int kSkillStatusSecret = 4;

// The difficulties and the last stage of each that UpdateSkillStatus() tests.
constexpr int kDifficultyEasy = 0;
constexpr int kDifficultyNormal = 1;
constexpr int kDifficultyExpert = 2;
constexpr int kEasyLastStage = 3;
constexpr int kNormalLastStage = 4;
constexpr int kExpertLastStage = 5;

} // namespace

// 0x0032b760
MetPersonaData::MetPersonaData() {
    mStats.RebuildLevelList();
    mUnknown154 = kInitialBirthday;
    mUnknown15c = 0;
    mUnknown160 = kNoText;
}

// 0x0032b880
void MetPersonaData::Save(OBStream *pStream) {
    const int nVersion = kRecordVersion;
    OBStream &stream = pStream->Write(&nVersion, sizeof(nVersion));
    mStats.Save(stream);
    mUnknown140.Save(stream);
    const unsigned nLength = mUnknown154.mLen;
    stream.Write(&nLength, sizeof(nLength));
    stream.WriteBytes(mUnknown154.mStr != nullptr ? mUnknown154.mStr : g_szEmptyString, nLength);
}

// 0x0032b968
void MetPersonaData::Load(IBStream *pStream) {
    int nVersion;
    pStream->Read(&nVersion, sizeof(nVersion));
    mStats.Load(*pStream);
    mUnknown140.Load(*pStream);
    mUnknown160 = mUnknown140.mUnknown00;

    if (nVersion <= kLastLegacyVersion) {
        // Yes, the binary constructs a controller mapping and a GameOptions record here only to
        // discard what it reads.
        ControllerConfig controllerConfig;
        std::vector<int> buttons;
        int nButtonCount;
        pStream->Read(&nButtonCount, sizeof(nButtonCount));
        buttons.resize(nButtonCount);
        for (std::vector<int>::iterator it = buttons.begin(); it != buttons.end(); ++it) {
            pStream->Read(&*it, sizeof(*it));
        }

        GameOptions options;
        options.Load(*pStream);

        int nStringCount;
        pStream->Read(&nStringCount, sizeof(nStringCount));
        for (int i = 0; i < nStringCount; ++i) {
            int nUnused;
            pStream->Read(&nUnused, sizeof(nUnused));
            HxStr text;
            unsigned nLength;
            pStream->Read(&nLength, sizeof(nLength));
            text.Alloc(nLength);
            pStream->ReadBytes(
                text.mStr != nullptr ? text.mStr : const_cast<char *>(g_szEmptyString), nLength);
            text.Clear();
        }
    }

    if (nVersion >= kBirthdayVersion) {
        unsigned nLength;
        pStream->Read(&nLength, sizeof(nLength));
        mUnknown154.Alloc(nLength);
        pStream->ReadBytes(mUnknown154.mStr != nullptr ? mUnknown154.mStr :
                                                         const_cast<char *>(g_szEmptyString),
                           nLength);
    }

    UpdateSkillStatus();
}

// 0x0032e1e8
void *MetPersonaData::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kAllocationTag);
}

// 0x0032e208
void MetPersonaData::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kAllocationTag);
}

// 0x0032e230
void MetPersonaData::SetName(const HxStr &name) {
    mUnknown140.mUnknown00 = name;
}

// 0x0032e278
MetPersonaData::~MetPersonaData() {
}

// 0x0032e308
void MetPersonaData::UpdateSkillStatus() {
    int nStatus;
    if (mStats.IsStageComplete(kDifficultyExpert, kExpertLastStage)) {
        nStatus = mStats.IsSecretUnlocked() ? kSkillStatusSecret : kSkillStatusExpert;
    } else if (mStats.IsStageComplete(kDifficultyNormal, kNormalLastStage)) {
        nStatus = kSkillStatusNormal;
    } else {
        nStatus = mStats.IsStageComplete(kDifficultyEasy, kEasyLastStage) != 0 ? kSkillStatusEasy :
                                                                                 kSkillStatusNone;
    }
    mUnknown140.SetSkillStatus(nStatus);
}

// 0x0032e488
void MetPersonaData::AttachToBurnSlot(int nSlot) {
    mUnknown140.AttachToBurnSlot(nSlot);
}

// 0x0032e258
int MetPersonaData::GetSkillStatus() {
    return mUnknown140.GetSkillStatus();
}

// 0x0032e380
void MetPersonaData::Print(std::ostream &stream) {
    stream << kStatsLabel;
    mStats.PrintLevels(stream);
    stream << kAppearanceLabel;
    mUnknown140.Print(stream);
    stream << kBirthdayLabel << mUnknown154 << kPrefabLabel << mUnknown15c;
}

// 0x0032e420
MetPersonaData &MetPersonaData::operator=(const MetPersonaData &other) {
    if (&other != this) {
        mUnknown140 = other.mUnknown140;
        mUnknown160 = other.mUnknown160;
        mStats.Assign(other.mStats);
        mUnknown154 = other.mUnknown154;
        mUnknown15c = other.mUnknown15c;
    }
    return *this;
}
