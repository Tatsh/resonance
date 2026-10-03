#include "met/metmultitips4screen.h"

#include "met/metstrings.h"
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

// A text object, the identifier the European release fills it from, and the configuration key the
// North American release fills it from.
struct TipText {
    const char *pszObject;
    MetStringId nId;
    const char *pszKey;
};

// The texts in the order slot 38 fills them. Yes, the panel takes the `tp3_panel` text.
static const TipText kTexts[] = {
    {"tp4_tips_pan.txt", kMetStrTp3Panel, "tp3_panel"},
    {"tp4_hud_head.txt", kMetStrTp4HudLabel, "tp4_hud_label"},
    {"tp4_desc_head.txt", kMetStrTp4DescLabel, "tp4_desc_label"},
    {"tp4_autocatcher_head.txt", kMetStrTp4Title1, "tp4_title_1"},
    {"tp4_autocatcher_val.txt", kMetStrTp4Help1, "tp4_help_1"},
    {"tp4_freestyler_head.txt", kMetStrTp4Title2, "tp4_title_2"},
    {"tp4_freestyler_val.txt", kMetStrTp4Help2, "tp4_help_2"},
    {"tp4_crippler_head.txt", kMetStrTp4Title3, "tp4_title_3"},
    {"tp4_crippler_val.txt", kMetStrTp4Help3, "tp4_help_3"},
    {"tp4_neutralizer_head.txt", kMetStrTp4Title4, "tp4_title_4"},
    {"tp4_neutralizer_val.txt", kMetStrTp4Help4, "tp4_help_4"},
    {"tp4_bumper_head.txt", kMetStrTp4Title5, "tp4_title_5"},
    {"tp4_bumper_val.txt", kMetStrTp4Help5, "tp4_help_5"},
};

// Yes, the binary does not test the text for null.
inline void FillText(const TipText &entry) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(entry.pszObject)));
    HxStr text = MetConfigText(entry.nId, kPromptConfigCode, entry.pszKey);
    pText->SetText(text);
}

} // namespace

// NTSC-U/C: 0x00309018, PAL: 0x0032e398
MetMultiTips4Screen::MetMultiTips4Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// NTSC-U/C: 0x003091e8, PAL: 0x0032e5e0
void MetMultiTips4Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry);
    }
}

// NTSC-U/C: 0x0030ddb8, PAL: 0x003339a8
MetMultiTips4Screen::~MetMultiTips4Screen() {
}

// NTSC-U/C: 0x0030de38, PAL: 0x00333a50
MetMultiTips4Screen *MetMultiTips4Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips4Screen(pRenderer, nPriority);
}
