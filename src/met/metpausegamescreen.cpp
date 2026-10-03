#include "met/metpausegamescreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metfrontendstate.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "pmgr";
static const char *const kDirectory = "metagame/Transition";
static const char *const kContainerName = "pause_multi";
static const char *const kPanelName = "MetPauseGameScreen";

// Counted from 1.
static const char *const kOptionTextFormat = "pmgr_opt%d.txt";
static const char *const kPausedText = "pmgr_paused.txt";

static const char *const kTutorialHeadingKey = "pause_tutorial";
static const char *const kRemixHeadingKey = "pause_remix";
static const char *const kGameHeadingKey = "pause_game";
static const char *const kGameLabelsKey = "pause_multi_game";
static const char *const kRemixLabelsKey = "pause_multi_remix";

constexpr int kOptionCount = 3;

constexpr int kPromptConfigCode = 0x258;
constexpr int kLabelsConfigCode = 0x259;

constexpr int kTutorialPhase = 5;

} // namespace

// NTSC-U/C: 0x0031c368, PAL: 0x00342550
MetPauseGameScreen::MetPauseGameScreen(MetRenderer *pRenderer, int nPriority)
    : MetPauseBaseScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mReturnPanel = kPanelName;
}

// NTSC-U/C: 0x0031c508, PAL: 0x00342750
void MetPauseGameScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    for (int i = 1; i <= kOptionCount; ++i) {
        Rnd::Text *pOption = dynamic_cast<Rnd::Text *>(
            Rnd::TheManager.Find(HxStr(Rnd::MakeString(kOptionTextFormat, i))));
        mOptionTexts.push_back(pOption);
    }
}

// NTSC-U/C: 0x0031c658, PAL: 0x003428c8
void MetPauseGameScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Rnd::Text *pPaused = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kPausedText)));

    MetStringId nHeadingId;
    const char *pszHeadingKey;
    if (MetFrontEndState::shared()->mPendingTransition == kTutorialPhase) {
        nHeadingId = kMetStrPauseTutorial;
        pszHeadingKey = kTutorialHeadingKey;
    } else if (params.mPlayMode == kPlayModeJam) {
        nHeadingId = kMetStrPauseRemix;
        pszHeadingKey = kRemixHeadingKey;
    } else {
        nHeadingId = kMetStrPauseGame;
        pszHeadingKey = kGameHeadingKey;
    }
    HxStr heading = MetConfigText(nHeadingId, kPromptConfigCode, pszHeadingKey);
    pPaused->SetText(heading);

    mOptionLabels.clear();
    const bool bGameLabels = MetFrontEndState::shared()->mPendingTransition == kTutorialPhase ||
                             params.mPlayMode == kPlayModeGame;
#ifdef VIDEO_STANDARD_PAL
    mOptionLabels.push_back(GetMetString(kMetStrPauseResume));
    mOptionLabels.push_back(GetMetString(kMetStrPauseQuit));
    if (bGameLabels) {
        mOptionLabels.push_back(GetMetString(kMetStrPauseRestart));
    }
#else
    QueryConfigStrings(
        &mOptionLabels, kLabelsConfigCode, bGameLabels ? kGameLabelsKey : kRemixLabelsKey);
#endif
    MetPauseBaseScreen::EnterAndShow();
}

// NTSC-U/C: 0x0031fa40, PAL: 0x003463f8
MetPauseGameScreen *MetPauseGameScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPauseGameScreen(pRenderer, nPriority);
}
