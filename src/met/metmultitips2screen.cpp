#include "met/metmultitips2screen.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "tp2";
static const char *const kContainerName = "multi_tip_02";
static const char *const kPreviousScreen = "MetMultiTips1Screen";
static const char *const kNextScreen = "MetMultiTips3Screen";
constexpr int kPage = 2;

constexpr int kPromptConfigCode = 0x258;

// Each text object and the configuration key it is filled from, in the order slot 38 fills them.
static const char *const kTexts[][2] = {
    {"tips_pan.txt", "tp_panel"},
    {"tp2_score_01.txt", "tp_score1"},
    {"tp2_score_02.txt", "tp_score2"},
    {"tp2_title_1.txt", "tp2_title_1"},
    {"tp2_help_1.txt", "tp2_help_1"},
    {"tp2_title_2.txt", "tp2_title_2"},
    {"tp2_help_2.txt", "tp2_help_2"},
};

// Yes, the binary does not test the text for null.
inline void FillText(const char *pszText, const char *pszKey) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszText)));
    HxStr text;
    QueryConfigString(&text, kPromptConfigCode, pszKey);
    pText->SetText(text);
}

} // namespace

// 0x00307d68
MetMultiTips2Screen::MetMultiTips2Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// 0x00307f38
void MetMultiTips2Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry[0], entry[1]);
    }
}

// 0x0030da98
MetMultiTips2Screen::~MetMultiTips2Screen() {
}

// 0x0030db18
MetMultiTips2Screen *MetMultiTips2Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips2Screen(pRenderer, nPriority);
}
