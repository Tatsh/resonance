#include "met/metmultitips3screen.h"

#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"

namespace {

static const char *const kScreenName = "tp3";
static const char *const kContainerName = "multi_tip_03";
static const char *const kPreviousScreen = "MetMultiTips2Screen";
static const char *const kNextScreen = "MetMultiTips4Screen";
constexpr int kPage = 3;

constexpr int kPromptConfigCode = 0x258;

// A text object, the identifier the European release fills it from, and the configuration key the
// North American release fills it from.
struct TipText {
    const char *pszObject;
    MetStringId nId;
    const char *pszKey;
};

// The texts in the order slot 38 fills them.
static const TipText kTexts[] = {
    {"tp3_tips_pan.txt", kMetStrTp3Panel, "tp3_panel"},
    {"tp3_power_notes.txt", kMetStrTp3Notes, "tp3_notes"},
    {"tp3_inventory.txt", kMetStrTp3Inventory, "tp3_inventory"},
    {"tp3_title_1.txt", kMetStrTp3Title1, "tp3_title_1"},
    {"tp3_help_1.txt", kMetStrTp3Help1, "tp3_help_1"},
    {"tp3_title_2.txt", kMetStrTp3Title2, "tp3_title_2"},
    {"tp3_help_2.txt", kMetStrTp3Help2, "tp3_help_2"},
};

// Yes, the binary does not test the text for null.
inline void FillText(const TipText &entry) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(entry.pszObject)));
    HxStr text = MetConfigText(entry.nId, kPromptConfigCode, entry.pszKey);
    pText->SetText(text);
}

} // namespace

// NTSC-U/C: 0x003086c0, PAL: 0x0032d870
MetMultiTips3Screen::MetMultiTips3Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// NTSC-U/C: 0x00308890, PAL: 0x0032dab8
void MetMultiTips3Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry);
    }
}

// NTSC-U/C: 0x0030dc28, PAL: 0x003337f0
MetMultiTips3Screen::~MetMultiTips3Screen() {
}

// NTSC-U/C: 0x0030dca8, PAL: 0x00333898
MetMultiTips3Screen *MetMultiTips3Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips3Screen(pRenderer, nPriority);
}
