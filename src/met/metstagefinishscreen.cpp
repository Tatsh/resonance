#include "met/metstagefinishscreen.h"

#include "app/application.h"
#include "game/campaignstats.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/gamestats.h"
#include "game/globalsettings.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/metglobalsettingssaverscreen.h"
#include "met/methelpscreen.h"
#include "met/metpersonadata.h"
#include "met/metpersonasaverscreen.h"
#include "met/metrenderer.h"
#include "met/metsolowinscreen.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "egc";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix. It is also the key the congratulation texts are
// looked up under.
static const char *const kContainerName = "end_game_congrats";

static const char *const kContinueButtonObject = "egc_continue.but";
static const char *const kContinuePrompt = "egsw_continue";

// The four congratulation texts ResolveContainerViews() fills, counted from 1.
static const char *const kCongratulationTextFormat = "egc_congrat%d.txt";
constexpr int kFirstCongratulationText = 1;
constexpr int kLastCongratulationText = 4;

// The keys the message builders queue.
static const char *const kHighScoreKey = "end_game_high_score";
static const char *const kArenaCompleteKey = "end_game_arena_complete";
static const char *const kStageScoreBeatKey = "stage_score_beat";
static const char *const kSecretKey = "end_game_secret";
static const char *const kSuperSecretKey = "end_game_super_secret";
static const char *const kEndSuperSecretKey = "end_game_end_super_secret";

// The panel slot 26 activates once the continue button shows.
static const char *const kPanelName = "MetStageFinishScreen";

// The empty literal that clears the panel and the prompt.
static const char *const kNoName = "";

// Configuration codes the messages and the arena names are read under.
constexpr int kPromptConfigCode = 600;
constexpr int kArenaNameConfigCode = 806;

// Frames between two steps of the message sequence.
constexpr float kMessageInterval = 360.0f;

// Index of the continue button, the one button in the ring.
constexpr int kContinueButtonIndex = 0;
// Sentinel MetButtonList::SetSelected() takes for no selection.
constexpr int kNoButton = -1;
// Rnd::Button state for a shown, unselected button.
constexpr int kButtonNormalState = 0;

// The selection alternation the select command starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// Rnd::Button state for a disabled button, which MetButtonList passes over.
constexpr int kButtonDisabledState = 3;

// The keys and formats of the stage and difficulty messages.
static const char *const kLastStageKey = "end_game_last_stage";
static const char *const kStageKey = "end_game_stage";
static const char *const kDifficultyUnlockKey = "end_game_easy_normal";

// The configuration code a level's stage is read under.
constexpr int kStageConfigCode = 0x25d;

// The difficulties, and the last stage each plays.
constexpr int kDifficultyEasy = 0;
constexpr int kDifficultyNormal = 1;
constexpr int kDifficultyExpert = 2;
constexpr int kEasyLastStage = 3;
constexpr int kNormalLastStage = 4;
constexpr int kExpertLastStage = 5;

// The views and texts ShowMessages() lays the messages out in.
static const char *const kLinesViewFormat = "egc_%dlines.view";
static const char *const kLinesView = "egc_lines.view";
static const char *const kMessageTextPrefixFormat = "egc_congrats_%dlines_0";
static const char *const kMessageTextFormat = "%s%d.txt";

// The screen slot 36 hands over to.
static const char *const kSoloWinScreen = "MetSoloWinScreen";

// The player whose score EnterAndShow() records.
constexpr int kFirstPlayer = 0;

// The secret stage, and the number of levels it has when its last level ends the super secret.
constexpr int kSecretStage = 6;
constexpr unsigned kSecretStageLevelCount = 2;

// The card location EnterAndShow() saves to when MetFrontEndState::mUsingMemcard is clear.
static const char *const kDefaultCardSlotName = "1";
constexpr int kDefaultCardSlotPort = 0;

// The value both MetFrontEndState flags must have for the global settings to be saved first.
constexpr int kFrontEndFlagSet = 1;

// Reports the text of a string, or the shared empty string when it has no buffer.
inline const char *TextOf(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Resolves a registry key to a Rnd::Text, or null.
inline Rnd::Text *FindText(const HxStr &name) {
    Rnd::Object *pObject = Rnd::g_manager.Find(name);
    return pObject != nullptr ? dynamic_cast<Rnd::Text *>(pObject) : nullptr;
}

// Resolves a registry key to a Rnd::View, or null.
inline Rnd::View *FindView(const HxStr &name) {
    Rnd::Object *pObject = Rnd::g_manager.Find(name);
    return pObject != nullptr ? dynamic_cast<Rnd::View *>(pObject) : nullptr;
}

} // namespace

// NTSC-U/C: 0x003bdbb8, PAL: 0x003f2908
MetStageFinishScreen::MetStageFinishScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mStageRecorded(0), mNextMessage(0), mLastStepTime(0.0f), mDifficultyUnlocked(0) {
    mContinueButtons = new MetButtonList();
    mShowsLoadedDrawables = 0;
}

// NTSC-U/C: 0x003c4250, PAL: 0x003f96d0
MetStageFinishScreen *MetStageFinishScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetStageFinishScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x003bded8, PAL: 0x003f2c98
MetStageFinishScreen::~MetStageFinishScreen() {
    delete mContinueButtons;
}

// NTSC-U/C: 0x003be068, PAL: 0x003f2e40
void MetStageFinishScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    {
        HxStr objectName(kContinueButtonObject);
        HxStr label = MetConfigText(kMetStrEgswContinue, kPromptConfigCode, kContinuePrompt);
        mContinueButtons->Add(objectName, label);
    }

    for (int i = kFirstCongratulationText; i <= kLastCongratulationText; ++i) {
        Rnd::Text *pText = FindText(HxStr(FormatString(kCongratulationTextFormat, i)));
        HxStr text = MetConfigText(kMetStrEndGameCongrats, kPromptConfigCode, kContainerName);
        pText->SetText(text); // The binary does not test the lookup for null.
    }
}

// NTSC-U/C: 0x003be2a8, PAL: 0x003f30e8
void MetStageFinishScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    if (mStageRecorded == 0 && !params.mLoadingGame) {
        MetPersonaData *pPersona = MetFrontEndState::shared()->GetFirstPersona();
        CampaignStats *pStats = &pPersona->mStats;
        GameStats *pGameStats = Application::shared()->GetGameManager()->GetStats();
        int nStage = QueryConfigValue(kStageConfigCode, TextOf(params.mLevelName));
        int nDifficulty = params.mDifficulty;

        int nWasBeaten = pStats->GetLevelBeaten(nDifficulty, params.mLevelName);
        int nOldHighScore = pStats->GetLevelHighScore(nDifficulty, params.mLevelName);
        int nOldUnlockLevel = pStats->mUnlockLevel;
        int nWasScoreBeaten = pStats->GetStageScoreBeaten(nDifficulty, nStage);
        int nWasStageComplete = pStats->IsStageComplete(nDifficulty, nStage);
        int nWasDifficultyComplete = pStats->IsDifficultyComplete(nDifficulty);
        int nWasSecret = pStats->IsSecretUnlocked();
        int nWasSuperSecret = pStats->IsSuperSecretUnlocked();

        pStats->RecordHighScore(nDifficulty, params.mLevelName, pGameStats->GetScore(kFirstPlayer));
        pStats->RecordLevelBeaten(nDifficulty, params.mLevelName);
        pPersona->UpdateSkillStatus();

        AddHighScoreMessage(nOldHighScore, pGameStats->GetScore(kFirstPlayer));
        AddArenaCompleteMessage(nOldUnlockLevel, pStats->mUnlockLevel);
        AddStageScoreBeatMessage(nWasScoreBeaten, pStats->GetStageScoreBeaten(nDifficulty, nStage));
        AddStageCompleteMessage(nWasStageComplete, pStats->IsStageComplete(nDifficulty, nStage));
        AddDifficultyUnlockMessage(nWasDifficultyComplete,
                                   pStats->IsDifficultyComplete(nDifficulty));
        AddSecretUnlockMessage(nWasSecret, pStats->IsSecretUnlocked());
        AddSuperSecretUnlockMessage(nWasSuperSecret, pStats->IsSuperSecretUnlocked());

        int nWasEndBeaten = 0;
        int nIsEndBeaten = 0;
        std::vector<StageListEntry> secretLevels(*GetStageList(kSecretStage));
        HxStr lastSecretLevel(kNoName);
        if (secretLevels.size() == kSecretStageLevelCount) {
            lastSecretLevel = secretLevels[kSecretStageLevelCount - 1].mName;
        }
        if (params.mLevelName == lastSecretLevel) {
            nWasEndBeaten = nWasBeaten;
            nIsEndBeaten = 1;
        }
        AddEndSuperSecretUnlockMessage(nWasEndBeaten, nIsEndBeaten);

        mStageRecorded = 1;
        if ((nWasBeaten == 0 || nOldHighScore < pGameStats->GetScore(kFirstPlayer)) &&
            MetFrontEndState::shared()->mUnlockAll == 0) {
            std::vector<HxStr> screens;
            screens.resize(1);
            screens[0] = kPanelName;
            if (MetFrontEndState::shared()->mUsingMemcard != 0) {
                GlobalSettings::shared(); // Yes, the binary discards this call's result.
                MetPersonaSaverScreen::StartSave(
                    screens, pPersona, GlobalSettings::shared()->mCardSlots[0], 0, 0);
            } else {
                MemcardConnectState slot;
                slot.mSlotName = kDefaultCardSlotName;
                slot.mPortSlot = kDefaultCardSlotPort;
                MetPersonaSaverScreen::StartSave(screens, pPersona, slot, 0, 0);
            }
            return;
        }
    }

    if (MetFrontEndState::shared()->mUsingMemcard == kFrontEndFlagSet &&
        MetFrontEndState::shared()->mSettingsDirty == kFrontEndFlagSet) {
        MetFrontEndState::shared()->mSettingsDirty = 0;
        std::vector<HxStr> screens;
        screens.push_back(HxStr(kPanelName));
        MetGlobalSettingsSaverScreen::StartSave(screens);
    } else {
        ShowMessages();
    }
}

// NTSC-U/C: 0x003bf788, PAL: 0x003f4840
void MetStageFinishScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (mLastStepTime != 0.0f || pCommand->mCommand != kMetScreenCommandSelect) {
        return;
    }
    ActivateNamedPanel(HxStr(kNoName));
    StartRepeatingSound(mRenderer->mAnimationFrame,
                        kSelectAlternateInterval,
                        mContinueButtons->mSelectedButton,
                        kSelectAlternateCycles);
    MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
}

// NTSC-U/C: 0x003c4228, PAL: 0x003f96a8
void MetStageFinishScreen::PlayLeaveSound(int) {
}

// NTSC-U/C: 0x003c4230, PAL: 0x003f96b0
void MetStageFinishScreen::PlayHighSound(int) {
}

// NTSC-U/C: 0x003c4238, PAL: 0x003f96b8
void MetStageFinishScreen::PlayCycleLeftSound(int) {
}

// NTSC-U/C: 0x003c4240, PAL: 0x003f96c0
void MetStageFinishScreen::PlayCycleRightSound(int) {
}

// NTSC-U/C: 0x003c4248, PAL: 0x003f96c8
void MetStageFinishScreen::PlayErrorSound(int) {
}

// NTSC-U/C: 0x003bf5e8, PAL: 0x003f4680
void MetStageFinishScreen::UpdateIdle(float flTime) {
    if (mLastStepTime == 0.0f || !(mLastStepTime + kMessageInterval < flTime)) {
        return;
    }
    if (static_cast<unsigned>(mNextMessage) < mMessages.size()) {
        mMessageDrawables[mNextMessage]->SetShowing(1);
    }
    ++mNextMessage;
    // The button shows one interval after the last message, which matches the binary.
    if (mMessages.size() < static_cast<unsigned>(mNextMessage)) {
        mContinueButtons->ButtonAt(kContinueButtonIndex)->SetShowing(1);
        mContinueButtons->ButtonAt(kContinueButtonIndex)->SetState(kButtonNormalState);
        mContinueButtons->SetSelected(kContinueButtonIndex);
        mLastStepTime = 0.0f;
        ActivateNamedPanel(HxStr(kPanelName));
    } else {
        mLastStepTime = flTime;
    }
}

// NTSC-U/C: 0x003c4390, PAL: 0x003f9830
void MetStageFinishScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    BeginExit();
}

// NTSC-U/C: 0x003c42d8, PAL: 0x003f9758
void MetStageFinishScreen::OnEnterFinished() {
    mContinueButtons->SetSelected(kNoButton);
    mLastStepTime = mRenderer->mAnimationFrame;
    ActivateNamedPanel(HxStr(kNoName));
}

// NTSC-U/C: 0x003bf8e8, PAL: 0x003f49e0
void MetStageFinishScreen::AddHighScoreMessage(int nPreviousScore, int nScore) {
    if (nPreviousScore == 0 || nPreviousScore >= nScore) {
        return;
    }
    HxStr message = MetConfigText(kMetStrEndGameHighScore, kPromptConfigCode, kHighScoreKey);
    mMessages.push_back(message);
}

// NTSC-U/C: 0x003bf9b8, PAL: 0x003f4ac8
void MetStageFinishScreen::AddArenaCompleteMessage(int nPreviousCompleted, int nCompleted) {
    if (nPreviousCompleted >= nCompleted) {
        return;
    }
    HxStr format = MetConfigText(kMetStrEndGameArenaComplete, kPromptConfigCode, kArenaCompleteKey);
    const ArenaListEntry &arena = (*GetArenaList())[nCompleted - 1];
    HxStr arenaName = QueryConfigString(kArenaNameConfigCode, TextOf(arena.mName));
    HxStr message(FormatString(TextOf(format), TextOf(arenaName)));
    mMessages.push_back(message);
}

// NTSC-U/C: 0x003bfc48, PAL: 0x003f4db8
void MetStageFinishScreen::AddStageCompleteMessage(int nWasComplete, int nIsComplete) {
    if (nIsComplete == 0 || nWasComplete != 0) {
        return;
    }
    MetFrontEndState::shared()->GetFirstPersona(); // Yes, the binary discards the persona.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    int nStage = QueryConfigValue(kStageConfigCode, TextOf(params.mLevelName));
    int nDifficulty = params.mDifficulty;
    if ((nStage == kEasyLastStage && nDifficulty == kDifficultyEasy) ||
        (nStage == kNormalLastStage && nDifficulty == kDifficultyNormal) ||
        (nStage == kExpertLastStage && nDifficulty == kDifficultyExpert)) {
        HxStr difficultyName = DifficultyName(nDifficulty);
        HxStr format = MetConfigText(kMetStrEndGameLastStage, kPromptConfigCode, kLastStageKey);
        HxStr message(FormatString(TextOf(format), TextOf(difficultyName)));
        mMessages.push_back(message);
    } else {
        HxStr format = MetConfigText(kMetStrEndGameStage, kPromptConfigCode, kStageKey);
        HxStr message(FormatString(TextOf(format), nStage + 1));
        mMessages.push_back(message);
    }
}

// NTSC-U/C: 0x003c0008, PAL: 0x003f5220
void MetStageFinishScreen::AddDifficultyUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    mDifficultyUnlocked = 0;
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    MetFrontEndState::shared()->GetFirstPersona(); // Yes, the binary discards the persona.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    QueryConfigValue(kStageConfigCode, TextOf(params.mLevelName)); // The stage is discarded.
    HxStr message;
    if (params.mDifficulty != kDifficultyExpert) {
        HxStr format =
            MetConfigText(kMetStrEndGameEasyNormal, kPromptConfigCode, kDifficultyUnlockKey);
        HxStr difficultyName = DifficultyName(params.mDifficulty + 1);
        message = FormatString(TextOf(format), TextOf(difficultyName));
        mMessages.push_back(message);
        mDifficultyUnlocked = 1;
    }
}

// NTSC-U/C: 0x003bef98, PAL: 0x003f3f70
void MetStageFinishScreen::ShowMessages() {
    if (mMessages.size() == 0) {
        BeginExit();
        return;
    }
    MetFrontEndState::shared()->GetFirstPersona(); // Yes, the binary discards the persona.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    QueryConfigValue(kStageConfigCode, TextOf(params.mLevelName)); // The stage is discarded.

    HxStr linesName(FormatString(kLinesViewFormat, mMessages.size()));
    Rnd::View *pCountLines = FindView(linesName);
    Rnd::View *pLines = FindView(HxStr(kLinesView));
    // The binary does not test the container view for null.
    pLines->ClearDraws();
    pLines->AddDraw(pCountLines, nullptr);

    for (int i = kFirstCongratulationText; i <= kLastCongratulationText; ++i) {
        FindText(HxStr(FormatString(kCongratulationTextFormat, i)))->SetShowing(1);
    }

    mMessageDrawables.clear();
    HxStr prefix(FormatString(kMessageTextPrefixFormat, mMessages.size()));
    for (unsigned i = 0; i < mMessages.size(); ++i) {
        HxStr name(FormatString(kMessageTextFormat, TextOf(prefix), i + 1));
        Rnd::Text *pText = FindText(name);
        pText->SetText(mMessages[i]);
        pText->SetShowing(0);
        mMessageDrawables.push_back(pText);
    }

    mContinueButtons->ButtonAt(kContinueButtonIndex)->SetShowing(0);
    mContinueButtons->ButtonAt(kContinueButtonIndex)->SetState(kButtonDisabledState);
    mContinueButtons->SetSelected(kContinueButtonIndex);
    mNextMessage = 0;
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x003c0530, PAL: 0x003f5820
void MetStageFinishScreen::OnExitFinished() {
    mStageRecorded = 0;
    MetSoloWinScreen::SetDifficultyUnlocked(mDifficultyUnlocked);
    PushNamedScreen(HxStr(kSoloWinScreen));
    ActivateNamedPanel(HxStr(kSoloWinScreen));
    mContinueButtons->SetSelected(kNoButton);
    mMessages.clear();
    for (int i = kFirstCongratulationText; i <= kLastCongratulationText; ++i) {
        FindText(HxStr(FormatString(kCongratulationTextFormat, i)))->SetShowing(0);
    }
}

// NTSC-U/C: 0x003bfb78, PAL: 0x003f4cd0
void MetStageFinishScreen::AddStageScoreBeatMessage(int nWasBeaten, int nIsBeaten) {
    if (nIsBeaten == 0 || nWasBeaten != 0) {
        return;
    }
    HxStr message = MetConfigText(kMetStrStageScoreBeat, kPromptConfigCode, kStageScoreBeatKey);
    mMessages.push_back(message);
}

// NTSC-U/C: 0x003c02c0, PAL: 0x003f5568
void MetStageFinishScreen::AddSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message = MetConfigText(kMetStrEndGameSecret, kPromptConfigCode, kSecretKey);
    mMessages.push_back(message);
}

// NTSC-U/C: 0x003c0390, PAL: 0x003f5650
void MetStageFinishScreen::AddSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message = MetConfigText(kMetStrEndGameSuperSecret, kPromptConfigCode, kSuperSecretKey);
    mMessages.push_back(message);
}

// NTSC-U/C: 0x003c0460, PAL: 0x003f5738
void MetStageFinishScreen::AddEndSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message =
        MetConfigText(kMetStrEndGameEndSuperSecret, kPromptConfigCode, kEndSuperSecretKey);
    mMessages.push_back(message);
}
