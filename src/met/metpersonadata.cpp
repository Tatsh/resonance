#include "met/metpersonadata.h"

namespace {

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
