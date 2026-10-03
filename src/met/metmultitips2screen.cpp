#include "met/metmultitips2screen.h"

#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"

namespace {

static const char *const kScreenName = "tp2";
static const char *const kContainerName = "multi_tip_02";
static const char *const kPreviousScreen = "MetMultiTips1Screen";
static const char *const kNextScreen = "MetMultiTips3Screen";
constexpr int kPage = 2;

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
    {"tips_pan.txt", kMetStrTpPanel, "tp_panel"},
    {"tp2_score_01.txt", kMetStrTpScore1, "tp_score1"},
    {"tp2_score_02.txt", kMetStrTpScore2, "tp_score2"},
    {"tp2_title_1.txt", kMetStrTp2Title1, "tp2_title_1"},
    {"tp2_help_1.txt", kMetStrTp2Help1, "tp2_help_1"},
    {"tp2_title_2.txt", kMetStrTp2Title2, "tp2_title_2"},
    {"tp2_help_2.txt", kMetStrTp2Help2, "tp2_help_2"},
};

// Yes, the binary does not test the text for null.
inline void FillText(const TipText &entry) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(entry.pszObject)));
    HxStr text = MetConfigText(entry.nId, kPromptConfigCode, entry.pszKey);
    pText->SetText(text);
}

} // namespace

// NTSC-U/C: 0x00307d68, PAL: 0x0032cd48
MetMultiTips2Screen::MetMultiTips2Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// NTSC-U/C: 0x00307f38, PAL: 0x0032cf90
void MetMultiTips2Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry);
    }
}

// NTSC-U/C: 0x0030da98, PAL: 0x00333638
MetMultiTips2Screen::~MetMultiTips2Screen() {
}

// NTSC-U/C: 0x0030db18, PAL: 0x003336e0
MetMultiTips2Screen *MetMultiTips2Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips2Screen(pRenderer, nPriority);
}
