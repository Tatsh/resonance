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

// NTSC-U/C: 0x0032b760, PAL: 0x00353bf0
MetPersonaData::MetPersonaData() {
    mStats.RebuildLevelList();
    mBirthday = kInitialBirthday;
    mIsPrefab = 0;
    mSavedName = kNoText;
}

// NTSC-U/C: 0x0032b880, PAL: 0x00353d40
void MetPersonaData::Save(OBStream *pStream) {
    const int nVersion = kRecordVersion;
    OBStream &stream = pStream->WriteLE(&nVersion, sizeof(nVersion));
    mStats.Save(stream);
    mAppearance.Save(stream);
    const unsigned nLength = mBirthday.mLen;
    stream.WriteLE(&nLength, sizeof(nLength));
    stream.Write(mBirthday.mStr != nullptr ? mBirthday.mStr : g_szEmptyString, nLength);
}

// NTSC-U/C: 0x0032b968, PAL: 0x00353e28
void MetPersonaData::Load(IBStream *pStream) {
    int nVersion;
    pStream->ReadLE(&nVersion, sizeof(nVersion));
    mStats.Load(*pStream);
    mAppearance.Load(*pStream);
    mSavedName = mAppearance.mUserName;

    if (nVersion <= kLastLegacyVersion) {
        // Yes, the binary constructs a controller mapping and a GameOptions record here only to
        // discard what it reads.
        ControllerConfig controllerConfig;
        std::vector<int> buttons;
        int nButtonCount;
        pStream->ReadLE(&nButtonCount, sizeof(nButtonCount));
        buttons.resize(nButtonCount);
        for (std::vector<int>::iterator it = buttons.begin(); it != buttons.end(); ++it) {
            pStream->ReadLE(&*it, sizeof(*it));
        }

        GameOptions options;
        options.Load(*pStream);

        int nStringCount;
        pStream->ReadLE(&nStringCount, sizeof(nStringCount));
        for (int i = 0; i < nStringCount; ++i) {
            int nUnused;
            pStream->ReadLE(&nUnused, sizeof(nUnused));
            HxStr text;
            unsigned nLength;
            pStream->ReadLE(&nLength, sizeof(nLength));
            text.Alloc(nLength);
            pStream->Read(text.mStr != nullptr ? text.mStr : const_cast<char *>(g_szEmptyString),
                          nLength);
            text.Clear();
        }
    }

    if (nVersion >= kBirthdayVersion) {
        unsigned nLength;
        pStream->ReadLE(&nLength, sizeof(nLength));
        mBirthday.Alloc(nLength);
        pStream->Read(mBirthday.mStr != nullptr ? mBirthday.mStr :
                                                  const_cast<char *>(g_szEmptyString),
                      nLength);
    }

    UpdateSkillStatus();
}

// NTSC-U/C: 0x0032e1e8, PAL: 0x003566f0
void *MetPersonaData::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kAllocationTag);
}

// NTSC-U/C: 0x0032e208, PAL: 0x00356710
void MetPersonaData::operator delete(void *pBlock) {
    OperatorDeleteOverride(pBlock, kAllocationTag);
}

// NTSC-U/C: 0x0032e230, PAL: 0x00356738
void MetPersonaData::SetName(const HxStr &name) {
    mAppearance.mUserName = name;
}

// NTSC-U/C: 0x0032e278, PAL: 0x00356780
MetPersonaData::~MetPersonaData() {
}

// NTSC-U/C: 0x0032e308, PAL: 0x00356838
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
    mAppearance.SetSkillStatus(nStatus);
}

// NTSC-U/C: 0x0032e488, PAL: 0x003569b8
void MetPersonaData::AttachToBurnSlot(int nSlot) {
    mAppearance.AttachToBurnSlot(nSlot);
}

// NTSC-U/C: 0x0032e258, PAL: 0x00356760
int MetPersonaData::GetSkillStatus() {
    return mAppearance.GetSkillStatus();
}

// NTSC-U/C: 0x0032e380, PAL: 0x003568b0
void MetPersonaData::Print(std::ostream &stream) {
    stream << kStatsLabel;
    mStats.PrintLevels(stream);
    stream << kAppearanceLabel;
    mAppearance.Print(stream);
    stream << kBirthdayLabel << mBirthday << kPrefabLabel << mIsPrefab;
}

// NTSC-U/C: 0x0032e420, PAL: 0x00356950
MetPersonaData &MetPersonaData::operator=(const MetPersonaData &other) {
    if (&other != this) {
        mAppearance = other.mAppearance;
        mSavedName = other.mSavedName;
        mStats.Assign(other.mStats);
        mBirthday = other.mBirthday;
        mIsPrefab = other.mIsPrefab;
    }
    return *this;
}
