#include "met/metmultitips3screen.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "tp3";
static const char *const kContainerName = "multi_tip_03";
static const char *const kPreviousScreen = "MetMultiTips2Screen";
static const char *const kNextScreen = "MetMultiTips4Screen";
constexpr int kPage = 3;

constexpr int kPromptConfigCode = 0x258;

// Each text object and the configuration key it is filled from, in the order slot 38 fills them.
static const char *const kTexts[][2] = {
    {"tp3_tips_pan.txt", "tp3_panel"},
    {"tp3_power_notes.txt", "tp3_notes"},
    {"tp3_inventory.txt", "tp3_inventory"},
    {"tp3_title_1.txt", "tp3_title_1"},
    {"tp3_help_1.txt", "tp3_help_1"},
    {"tp3_title_2.txt", "tp3_title_2"},
    {"tp3_help_2.txt", "tp3_help_2"},
};

// Yes, the binary does not test the text for null.
inline void FillText(const char *pszText, const char *pszKey) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszText)));
    HxStr text;
    QueryConfigString(&text, kPromptConfigCode, pszKey);
    pText->SetText(text);
}

} // namespace

// 0x003086c0
MetMultiTips3Screen::MetMultiTips3Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// 0x00308890
void MetMultiTips3Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry[0], entry[1]);
    }
}

// 0x0030dc28
MetMultiTips3Screen::~MetMultiTips3Screen() {
}

// 0x0030dca8
MetMultiTips3Screen *MetMultiTips3Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips3Screen(pRenderer, nPriority);
}
