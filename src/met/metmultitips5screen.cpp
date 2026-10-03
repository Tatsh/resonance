#include "met/metmultitips5screen.h"

#include "met/metstrings.h"
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

// A text object, the identifier the European release fills it from, and the configuration key the
// North American release fills it from.
struct TipText {
    const char *pszObject;
    MetStringId nId;
    const char *pszKey;
};

// The texts in the order slot 38 fills them.
static const TipText kTexts[] = {
    {"tp5_tips_pan.txt", kMetStrTp5Panel, "tp5_panel"},
    {"tp5_title.txt", kMetStrTp5Title, "tp5_title"},
    {"tp5_help.txt", kMetStrTp5Help, "tp5_help"},
#ifdef VIDEO_STANDARD_PAL
    {"tp5_track_01.txt", kMetStrIngSYNTH, nullptr},
    {"tp5_track_02.txt", kMetStrIngBASS, nullptr},
#endif
};

// Yes, the binary does not test the text for null.
inline void FillText(const TipText &entry) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(entry.pszObject)));
    HxStr text = MetConfigText(entry.nId, kPromptConfigCode, entry.pszKey);
    pText->SetText(text);
}

} // namespace

// NTSC-U/C: 0x00309fa0, PAL: 0x0032f610
MetMultiTips5Screen::MetMultiTips5Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// NTSC-U/C: 0x0030a170, PAL: 0x0032f858
void MetMultiTips5Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry);
    }
}

// NTSC-U/C: 0x0030a4d8, PAL: 0x0032fec8
void MetMultiTips5Screen::OnExitFinished() {
    if (mExitChoice == kExitNext) {
        ReturnToPlayerCount();
    } else {
        MetMultiTipsBaseScreen::OnExitFinished();
    }
}

// NTSC-U/C: 0x0030df48, PAL: 0x00333b60
MetMultiTips5Screen::~MetMultiTips5Screen() {
}

// NTSC-U/C: 0x0030dfc8, PAL: 0x00333c08
MetMultiTips5Screen *MetMultiTips5Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips5Screen(pRenderer, nPriority);
}
