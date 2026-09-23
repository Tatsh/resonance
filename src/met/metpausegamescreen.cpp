#include "met/metpausegamescreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metfrontendstate.h"
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

// 0x0031c368
MetPauseGameScreen::MetPauseGameScreen(MetRenderer *pRenderer, int nPriority)
    : MetPauseBaseScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mUnknown9c = kPanelName;
}

// 0x0031c508
void MetPauseGameScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    for (int i = 1; i <= kOptionCount; ++i) {
        Rnd::Text *pOption = dynamic_cast<Rnd::Text *>(
            Rnd::g_manager.Find(HxStr(FormatString(kOptionTextFormat, i))));
        mUnknowna4.push_back(pOption);
    }
}

// 0x0031c658
void MetPauseGameScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Rnd::Text *pPaused = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kPausedText)));

    const char *pszHeadingKey;
    if (MetFrontEndState::shared()->mUnknown18 == kTutorialPhase) {
        pszHeadingKey = kTutorialHeadingKey;
    } else if (params.mUnknown1c == kPlayModeJam) {
        pszHeadingKey = kRemixHeadingKey;
    } else {
        pszHeadingKey = kGameHeadingKey;
    }
    HxStr heading;
    QueryConfigString(&heading, kPromptConfigCode, pszHeadingKey);
    pPaused->SetText(heading);

    mUnknown90.clear();
    const bool bGameLabels = MetFrontEndState::shared()->mUnknown18 == kTutorialPhase ||
                             params.mUnknown1c == kPlayModeGame;
    QueryConfigStrings(
        &mUnknown90, kLabelsConfigCode, bGameLabels ? kGameLabelsKey : kRemixLabelsKey);
    MetPauseBaseScreen::EnterAndShow();
}

// 0x0031fa40
MetPauseGameScreen *MetPauseGameScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPauseGameScreen(pRenderer, nPriority);
}
