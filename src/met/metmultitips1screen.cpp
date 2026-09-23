#include "met/metmultitips1screen.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "tp1";
static const char *const kContainerName = "multi_tip_01";
static const char *const kPreviousScreen = "MetLocNumPlayersScreen";
static const char *const kNextScreen = "MetMultiTips2Screen";
constexpr int kPage = 1;

constexpr int kPromptConfigCode = 0x258;

// Each text object and the configuration key it is filled from, in the order slot 38 fills them.
static const char *const kTexts[][2] = {
    {"tips_pan.txt", "tp_panel"},
    {"tp_activator_01.txt", "tp_activator1"},
    {"tp_activator_02.txt", "tp_activator2"},
    {"tp1_title.txt", "tp1_title"},
    {"tp1_help.txt", "tp1_help"},
};

// Yes, the binary does not test the text for null.
inline void FillText(const char *pszText, const char *pszKey) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszText)));
    HxStr text;
    QueryConfigString(&text, kPromptConfigCode, pszKey);
    pText->SetText(text);
}

} // namespace

// 0x00307480
MetMultiTips1Screen::MetMultiTips1Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// 0x00307650
void MetMultiTips1Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry[0], entry[1]);
    }
}

// 0x00307bc8
void MetMultiTips1Screen::OnUnknownSlot36() {
    if (mUnknown18 == kExitPrevious) {
        ReturnToPlayerCount();
    } else {
        MetMultiTipsBaseScreen::OnUnknownSlot36();
    }
}

// 0x0030d908
MetMultiTips1Screen::~MetMultiTips1Screen() {
}

// 0x0030d988
MetMultiTips1Screen *MetMultiTips1Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips1Screen(pRenderer, nPriority);
}
