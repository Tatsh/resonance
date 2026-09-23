#include "met/metmultitips5screen.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "tp5";
static const char *const kContainerName = "multi_tip_05";
static const char *const kPreviousScreen = "MetMultiTips4Screen";
static const char *const kNextScreen = "MetLocNumPlayersScreen";
constexpr int kPage = 5;

constexpr int kPromptConfigCode = 0x258;

// Each text object and the configuration key it is filled from, in the order slot 38 fills them.
static const char *const kTexts[][2] = {
    {"tp5_tips_pan.txt", "tp5_panel"},
    {"tp5_title.txt", "tp5_title"},
    {"tp5_help.txt", "tp5_help"},
};

// Yes, the binary does not test the text for null.
inline void FillText(const char *pszText, const char *pszKey) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszText)));
    HxStr text = QueryConfigString(kPromptConfigCode, pszKey);
    pText->SetText(text);
}

} // namespace

// 0x00309fa0
MetMultiTips5Screen::MetMultiTips5Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// 0x0030a170
void MetMultiTips5Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry[0], entry[1]);
    }
}

// 0x0030a4d8
void MetMultiTips5Screen::OnUnknownSlot36() {
    if (mUnknown18 == kExitNext) {
        ReturnToPlayerCount();
    } else {
        MetMultiTipsBaseScreen::OnUnknownSlot36();
    }
}

// 0x0030df48
MetMultiTips5Screen::~MetMultiTips5Screen() {
}

// 0x0030dfc8
MetMultiTips5Screen *MetMultiTips5Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips5Screen(pRenderer, nPriority);
}
