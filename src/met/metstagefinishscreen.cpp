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
constexpr int kPromptConfigCode = 0x258;
constexpr int kArenaNameConfigCode = 0x326;

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

// The card location EnterAndShow() saves to when MetFrontEndState::mUnknown0c is clear.
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

// 0x003bdbb8
MetStageFinishScreen::MetStageFinishScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknowna8(0), mUnknownac(0), mUnknownb0(0.0f), mUnknownb4(0) {
    mUnknowna4 = new MetButtonList();
    mUnknown60 = 0;
}

// 0x003c4250
MetStageFinishScreen *MetStageFinishScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetStageFinishScreen(pRenderer, nPriority);
}

// 0x003bded8
MetStageFinishScreen::~MetStageFinishScreen() {
    delete mUnknowna4;
}

// 0x003be068
void MetStageFinishScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    {
        HxStr objectName(kContinueButtonObject);
        HxStr label = QueryConfigString(kPromptConfigCode, kContinuePrompt);
        mUnknowna4->Add(objectName, label);
    }

    for (int i = kFirstCongratulationText; i <= kLastCongratulationText; ++i) {
        Rnd::Text *pText = FindText(HxStr(FormatString(kCongratulationTextFormat, i)));
        HxStr text = QueryConfigString(kPromptConfigCode, kContainerName);
        pText->SetText(text); // The binary does not test the lookup for null.
    }
}

// 0x003be2a8
void MetStageFinishScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    if (mUnknowna8 == 0 && !params.mLoadingGame) {
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

        mUnknowna8 = 1;
        if ((nWasBeaten == 0 || nOldHighScore < pGameStats->GetScore(kFirstPlayer)) &&
            MetFrontEndState::shared()->mUnknown14 == 0) {
            std::vector<HxStr> screens;
            screens.resize(1);
            screens[0] = kPanelName;
            if (MetFrontEndState::shared()->mUnknown0c != 0) {
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

    if (MetFrontEndState::shared()->mUnknown0c == kFrontEndFlagSet &&
        MetFrontEndState::shared()->mUnknown10 == kFrontEndFlagSet) {
        MetFrontEndState::shared()->mUnknown10 = 0;
        std::vector<HxStr> screens;
        screens.push_back(HxStr(kPanelName));
        MetGlobalSettingsSaverScreen::StartSave(screens);
    } else {
        ShowMessages();
    }
}

// 0x003bf788
void MetStageFinishScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (mUnknownb0 != 0.0f || pCommand->mCommand != kMetScreenCommandSelect) {
        return;
    }
    ActivateNamedPanel(HxStr(kNoName));
    StartRepeatingSound(mUnknown10->mUnknown68,
                        kSelectAlternateInterval,
                        mUnknowna4->mUnknown00,
                        kSelectAlternateCycles);
    MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
}

// 0x003c4228
void MetStageFinishScreen::PlayLeaveSound(int) {
}

// 0x003c4230
void MetStageFinishScreen::PlayHighSound(int) {
}

// 0x003c4238
void MetStageFinishScreen::PlayCycleLeftSound(int) {
}

// 0x003c4240
void MetStageFinishScreen::PlayCycleRightSound(int) {
}

// 0x003c4248
void MetStageFinishScreen::PlayErrorSound(int) {
}

// 0x003bf5e8
void MetStageFinishScreen::OnUnknownSlot26(float flTime) {
    if (mUnknownb0 == 0.0f || !(mUnknownb0 + kMessageInterval < flTime)) {
        return;
    }
    if (static_cast<unsigned>(mUnknownac) < mUnknown8c.size()) {
        mUnknown98[mUnknownac]->SetShowing(1);
    }
    ++mUnknownac;
    // The button shows one interval after the last message, which matches the binary.
    if (mUnknown8c.size() < static_cast<unsigned>(mUnknownac)) {
        mUnknowna4->ButtonAt(kContinueButtonIndex)->SetShowing(1);
        mUnknowna4->ButtonAt(kContinueButtonIndex)->SetState(kButtonNormalState);
        mUnknowna4->SetSelected(kContinueButtonIndex);
        mUnknownb0 = 0.0f;
        ActivateNamedPanel(HxStr(kPanelName));
    } else {
        mUnknownb0 = flTime;
    }
}

// 0x003c4390
void MetStageFinishScreen::OnUnknownSlot30(Rnd::Button *) {
    BeginExit();
}

// 0x003c42d8
void MetStageFinishScreen::OnUnknownSlot33() {
    mUnknowna4->SetSelected(kNoButton);
    mUnknownb0 = mUnknown10->mUnknown68;
    ActivateNamedPanel(HxStr(kNoName));
}

// 0x003bf8e8
void MetStageFinishScreen::AddHighScoreMessage(int nPreviousScore, int nScore) {
    if (nPreviousScore == 0 || nPreviousScore >= nScore) {
        return;
    }
    HxStr message = QueryConfigString(kPromptConfigCode, kHighScoreKey);
    mUnknown8c.push_back(message);
}

// 0x003bf9b8
void MetStageFinishScreen::AddArenaCompleteMessage(int nPreviousCompleted, int nCompleted) {
    if (nPreviousCompleted >= nCompleted) {
        return;
    }
    HxStr format = QueryConfigString(kPromptConfigCode, kArenaCompleteKey);
    const ArenaListEntry &arena = (*GetArenaList())[nCompleted - 1];
    HxStr arenaName = QueryConfigString(kArenaNameConfigCode, TextOf(arena.mName));
    HxStr message(FormatString(TextOf(format), TextOf(arenaName)));
    mUnknown8c.push_back(message);
}

// 0x003bfc48
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
        HxStr format = QueryConfigString(kPromptConfigCode, kLastStageKey);
        HxStr message(FormatString(TextOf(format), TextOf(difficultyName)));
        mUnknown8c.push_back(message);
    } else {
        HxStr format = QueryConfigString(kPromptConfigCode, kStageKey);
        HxStr message(FormatString(TextOf(format), nStage + 1));
        mUnknown8c.push_back(message);
    }
}

// 0x003c0008
void MetStageFinishScreen::AddDifficultyUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    mUnknownb4 = 0;
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    MetFrontEndState::shared()->GetFirstPersona(); // Yes, the binary discards the persona.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    QueryConfigValue(kStageConfigCode, TextOf(params.mLevelName)); // The stage is discarded.
    HxStr message;
    if (params.mDifficulty != kDifficultyExpert) {
        HxStr format = QueryConfigString(kPromptConfigCode, kDifficultyUnlockKey);
        HxStr difficultyName = DifficultyName(params.mDifficulty + 1);
        message = FormatString(TextOf(format), TextOf(difficultyName));
        mUnknown8c.push_back(message);
        mUnknownb4 = 1;
    }
}

// 0x003bef98
void MetStageFinishScreen::ShowMessages() {
    if (mUnknown8c.size() == 0) {
        BeginExit();
        return;
    }
    MetFrontEndState::shared()->GetFirstPersona(); // Yes, the binary discards the persona.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    QueryConfigValue(kStageConfigCode, TextOf(params.mLevelName)); // The stage is discarded.

    HxStr linesName(FormatString(kLinesViewFormat, mUnknown8c.size()));
    Rnd::View *pCountLines = FindView(linesName);
    Rnd::View *pLines = FindView(HxStr(kLinesView));
    // The binary does not test the container view for null.
    pLines->ClearDraws();
    pLines->AddDraw(pCountLines, nullptr);

    for (int i = kFirstCongratulationText; i <= kLastCongratulationText; ++i) {
        FindText(HxStr(FormatString(kCongratulationTextFormat, i)))->SetShowing(1);
    }

    mUnknown98.clear();
    HxStr prefix(FormatString(kMessageTextPrefixFormat, mUnknown8c.size()));
    for (unsigned i = 0; i < mUnknown8c.size(); ++i) {
        HxStr name(FormatString(kMessageTextFormat, TextOf(prefix), i + 1));
        Rnd::Text *pText = FindText(name);
        pText->SetText(mUnknown8c[i]);
        pText->SetShowing(0);
        mUnknown98.push_back(pText);
    }

    mUnknowna4->ButtonAt(kContinueButtonIndex)->SetShowing(0);
    mUnknowna4->ButtonAt(kContinueButtonIndex)->SetState(kButtonDisabledState);
    mUnknowna4->SetSelected(kContinueButtonIndex);
    mUnknownac = 0;
    MetScreen::EnterAndShow();
}

// 0x003c0530
void MetStageFinishScreen::OnUnknownSlot36() {
    mUnknowna8 = 0;
    MetSoloWinScreen::SetDifficultyUnlocked(mUnknownb4);
    PushNamedScreen(HxStr(kSoloWinScreen));
    ActivateNamedPanel(HxStr(kSoloWinScreen));
    mUnknowna4->SetSelected(kNoButton);
    mUnknown8c.clear();
    for (int i = kFirstCongratulationText; i <= kLastCongratulationText; ++i) {
        FindText(HxStr(FormatString(kCongratulationTextFormat, i)))->SetShowing(0);
    }
}

// 0x003bfb78
void MetStageFinishScreen::AddStageScoreBeatMessage(int nWasBeaten, int nIsBeaten) {
    if (nIsBeaten == 0 || nWasBeaten != 0) {
        return;
    }
    HxStr message = QueryConfigString(kPromptConfigCode, kStageScoreBeatKey);
    mUnknown8c.push_back(message);
}

// 0x003c02c0
void MetStageFinishScreen::AddSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message = QueryConfigString(kPromptConfigCode, kSecretKey);
    mUnknown8c.push_back(message);
}

// 0x003c0390
void MetStageFinishScreen::AddSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message = QueryConfigString(kPromptConfigCode, kSuperSecretKey);
    mUnknown8c.push_back(message);
}

// 0x003c0460
void MetStageFinishScreen::AddEndSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message = QueryConfigString(kPromptConfigCode, kEndSuperSecretKey);
    mUnknown8c.push_back(message);
}
