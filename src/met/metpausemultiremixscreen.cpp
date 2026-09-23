#include "met/metpausemultiremixscreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

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

// 0x00327d90
MetPauseMultiRemixScreen::MetPauseMultiRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetPauseBaseScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mUnknown9c = kPanelName;
}

// 0x00327f30
void MetPauseMultiRemixScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    for (int i = 1; i <= kOptionCount; ++i) {
        Rnd::Text *pOption = dynamic_cast<Rnd::Text *>(
            Rnd::g_manager.Find(HxStr(FormatString(kOptionTextFormat, i))));
        mUnknowna4.push_back(pOption);
    }
}

// 0x00328080
void MetPauseMultiRemixScreen::EnterAndShow() {
    // Yes, the binary copies the settings and never reads the copy.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Rnd::Text *pPaused = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kPausedText)));

    HxStr heading;
    QueryConfigString(&heading, kPromptConfigCode, kHeadingKey);
    pPaused->SetText(heading);

    mUnknown90.clear();
    QueryConfigStrings(&mUnknown90, kLabelsConfigCode, kLabelsKey);
    // Yes, the binary copies the labels here and again in the base slot.
    for (std::vector<HxStr>::size_type i = 0; i < mUnknown90.size(); ++i) {
        mUnknowna4[i]->SetText(mUnknown90[i]);
    }
    MetPauseBaseScreen::EnterAndShow();
}

// 0x0032b3c8
MetPauseMultiRemixScreen *MetPauseMultiRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPauseMultiRemixScreen(pRenderer, nPriority);
}

// 0x0032b4a8
void MetPauseMultiRemixScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandBack || pCommand->mCommand == kCommandResume) {
        MetPauseBaseScreen::HandleCommand(pCommand);
    }
}
