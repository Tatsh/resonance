#include "met/metstagefinishscreen.h"

#include "met/metbuttonlist.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metsonglists.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/text.h"
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

} // namespace

MetStageFinishScreen::MetStageFinishScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknowna8(0), mUnknownac(0), mUnknownb0(0.0f), mUnknownb4(0) {
    mUnknowna4 = new MetButtonList();
    mUnknown60 = 0;
}

MetStageFinishScreen::~MetStageFinishScreen() {
    delete mUnknowna4;
}

void MetStageFinishScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    {
        HxStr objectName(kContinueButtonObject);
        HxStr label;
        QueryConfigString(&label, kPromptConfigCode, kContinuePrompt);
        mUnknowna4->Add(objectName, label);
    }

    for (int i = kFirstCongratulationText; i <= kLastCongratulationText; ++i) {
        Rnd::Text *pText;
        {
            HxStr name(FormatString(kCongratulationTextFormat, i));
            Rnd::Object *pObject = Rnd::g_manager.Find(name);
            pText = pObject != nullptr ? dynamic_cast<Rnd::Text *>(pObject) : nullptr;
        }
        HxStr text;
        QueryConfigString(&text, kPromptConfigCode, kContainerName);
        pText->SetText(text); // The binary does not test the lookup for null.
    }
}

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

void MetStageFinishScreen::PlayLeaveSound() {
}

void MetStageFinishScreen::PlayHighSound(int) {
}

void MetStageFinishScreen::PlayCycleLeftSound(int) {
}

void MetStageFinishScreen::PlayCycleRightSound(int) {
}

void MetStageFinishScreen::PlayErrorSound(int) {
}

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

void MetStageFinishScreen::OnUnknownSlot30(Rnd::Object *) {
    BeginExit();
}

void MetStageFinishScreen::OnUnknownSlot33() {
    mUnknowna4->SetSelected(kNoButton);
    mUnknownb0 = mUnknown10->mUnknown68;
    ActivateNamedPanel(HxStr(kNoName));
}

void MetStageFinishScreen::AddHighScoreMessage(int nPreviousScore, int nScore) {
    if (nPreviousScore == 0 || nPreviousScore >= nScore) {
        return;
    }
    HxStr message;
    QueryConfigString(&message, kPromptConfigCode, kHighScoreKey);
    mUnknown8c.push_back(message);
}

void MetStageFinishScreen::AddArenaCompleteMessage(int nPreviousCompleted, int nCompleted) {
    if (nPreviousCompleted >= nCompleted) {
        return;
    }
    HxStr format;
    QueryConfigString(&format, kPromptConfigCode, kArenaCompleteKey);
    const ArenaListEntry &arena = (*GetArenaList())[nCompleted - 1];
    HxStr arenaName;
    QueryConfigString(&arenaName,
                      kArenaNameConfigCode,
                      arena.mName.mStr != nullptr ? arena.mName.mStr : g_szEmptyString);
    HxStr message(FormatString(format.mStr != nullptr ? format.mStr : g_szEmptyString,
                               arenaName.mStr != nullptr ? arenaName.mStr : g_szEmptyString));
    mUnknown8c.push_back(message);
}

void MetStageFinishScreen::AddStageScoreBeatMessage(int nWasBeaten, int nIsBeaten) {
    if (nIsBeaten == 0 || nWasBeaten != 0) {
        return;
    }
    HxStr message;
    QueryConfigString(&message, kPromptConfigCode, kStageScoreBeatKey);
    mUnknown8c.push_back(message);
}

void MetStageFinishScreen::AddSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message;
    QueryConfigString(&message, kPromptConfigCode, kSecretKey);
    mUnknown8c.push_back(message);
}

void MetStageFinishScreen::AddSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message;
    QueryConfigString(&message, kPromptConfigCode, kSuperSecretKey);
    mUnknown8c.push_back(message);
}

void MetStageFinishScreen::AddEndSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked) {
    if (nIsUnlocked == 0 || nWasUnlocked != 0) {
        return;
    }
    HxStr message;
    QueryConfigString(&message, kPromptConfigCode, kEndSuperSecretKey);
    mUnknown8c.push_back(message);
}
