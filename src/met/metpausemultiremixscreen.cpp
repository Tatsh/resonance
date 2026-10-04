#include "met/metpausemultiremixscreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"

namespace {

static const char *const kScreenName = "pngr";
static const char *const kDirectory = "metagame/Transition";
static const char *const kContainerName = "pause_net";
static const char *const kPanelName = "MetPauseMultiRemixScreen";

// Counted from 1.
static const char *const kOptionTextFormat = "pngr_opt%d.txt";
static const char *const kPausedText = "psr_paused.txt";

static const char *const kHeadingKey = "pause_remix";
static const char *const kLabelsKey = "pause_multi_remix";

constexpr int kOptionCount = 2;

constexpr int kPromptConfigCode = 0x258;
constexpr int kLabelsConfigCode = 0x259;

// A command code past MetScreenCommandCode's range, which a pause screen treats as a resume.
constexpr int kCommandResume = 10;

} // namespace

MetPauseMultiRemixScreen::MetPauseMultiRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetPauseBaseScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mReturnPanel = kPanelName;
}

void MetPauseMultiRemixScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    for (int i = 1; i <= kOptionCount; ++i) {
        Rnd::Text *pOption = dynamic_cast<Rnd::Text *>(
            Rnd::TheManager.Find(HxStr(Rnd::MakeString(kOptionTextFormat, i))));
        mOptionTexts.push_back(pOption);
    }
}

void MetPauseMultiRemixScreen::EnterAndShow() {
    // Yes, the binary copies the settings and never reads the copy.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Rnd::Text *pPaused = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kPausedText)));

    HxStr heading = MetConfigText(kMetStrPauseRemix, kPromptConfigCode, kHeadingKey);
    pPaused->SetText(heading);

    mOptionLabels.clear();
#ifdef VIDEO_STANDARD_PAL
    mOptionLabels.push_back(GetMetString(kMetStrPauseResume));
    mOptionLabels.push_back(GetMetString(kMetStrPauseQuit));
#else
    QueryConfigStrings(&mOptionLabels, kLabelsConfigCode, kLabelsKey);
#endif
    // Yes, the binary copies the labels here and again in the base slot.
    for (std::vector<HxStr>::size_type i = 0; i < mOptionLabels.size(); ++i) {
        mOptionTexts[i]->SetText(mOptionLabels[i]);
    }
    MetPauseBaseScreen::EnterAndShow();
}

MetPauseMultiRemixScreen *MetPauseMultiRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPauseMultiRemixScreen(pRenderer, nPriority);
}

void MetPauseMultiRemixScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandBack || pCommand->mCommand == kCommandResume) {
        MetPauseBaseScreen::HandleCommand(pCommand);
    }
}
