#include "met/metmultitips4screen.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "tp4";
static const char *const kContainerName = "multi_tip_04";
static const char *const kPreviousScreen = "MetMultiTips3Screen";
static const char *const kNextScreen = "MetMultiTips5Screen";
constexpr int kPage = 4;

constexpr int kPromptConfigCode = 0x258;

// Each text object and the configuration key it is filled from, in the order slot 38 fills them.
// Yes, the panel takes the `tp3_panel` key.
static const char *const kTexts[][2] = {
    {"tp4_tips_pan.txt", "tp3_panel"},
    {"tp4_hud_head.txt", "tp4_hud_label"},
    {"tp4_desc_head.txt", "tp4_desc_label"},
    {"tp4_autocatcher_head.txt", "tp4_title_1"},
    {"tp4_autocatcher_val.txt", "tp4_help_1"},
    {"tp4_freestyler_head.txt", "tp4_title_2"},
    {"tp4_freestyler_val.txt", "tp4_help_2"},
    {"tp4_crippler_head.txt", "tp4_title_3"},
    {"tp4_crippler_val.txt", "tp4_help_3"},
    {"tp4_neutralizer_head.txt", "tp4_title_4"},
    {"tp4_neutralizer_val.txt", "tp4_help_4"},
    {"tp4_bumper_head.txt", "tp4_title_5"},
    {"tp4_bumper_val.txt", "tp4_help_5"},
};

// Yes, the binary does not test the text for null.
inline void FillText(const char *pszText, const char *pszKey) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszText)));
    HxStr text;
    QueryConfigString(&text, kPromptConfigCode, pszKey);
    pText->SetText(text);
}

} // namespace

// 0x00309018
MetMultiTips4Screen::MetMultiTips4Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// 0x003091e8
void MetMultiTips4Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry[0], entry[1]);
    }
}

// 0x0030ddb8
MetMultiTips4Screen::~MetMultiTips4Screen() {
}

// 0x0030de38
MetMultiTips4Screen *MetMultiTips4Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips4Screen(pRenderer, nPriority);
}
