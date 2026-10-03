#include "met/metmultitips1screen.h"

#include "met/metstrings.h"
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
    {"tp_activator_01.txt", kMetStrTpActivator1, "tp_activator1"},
    {"tp_activator_02.txt", kMetStrTpActivator2, "tp_activator2"},
    {"tp1_title.txt", kMetStrTp1Title, "tp1_title"},
    {"tp1_help.txt", kMetStrTp1Help, "tp1_help"},
#ifdef VIDEO_STANDARD_PAL
    {"tp1_track_01.txt", kMetStrIngSYNTH, nullptr},
    {"tp1_track_02.txt", kMetStrIngBASS, nullptr},
#endif
};

// Yes, the binary does not test the text for null.
inline void FillText(const TipText &entry) {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(entry.pszObject)));
    HxStr text = MetConfigText(entry.nId, kPromptConfigCode, entry.pszKey);
    pText->SetText(text);
}

} // namespace

// NTSC-U/C: 0x00307480, PAL: 0x0032c020
MetMultiTips1Screen::MetMultiTips1Screen(MetRenderer *pRenderer, int nPriority)
    : MetMultiTipsBaseScreen(pRenderer,
                             nPriority,
                             HxStr(kScreenName),
                             HxStr(kContainerName),
                             kPage,
                             HxStr(kPreviousScreen),
                             HxStr(kNextScreen)) {
}

// NTSC-U/C: 0x00307650, PAL: 0x0032c268
void MetMultiTips1Screen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    // The binary expands this loop into one call per text.
    for (const auto &entry : kTexts) {
        FillText(entry);
    }
}

// NTSC-U/C: 0x00307bc8, PAL: 0x0032cb48
void MetMultiTips1Screen::OnExitFinished() {
    if (mExitChoice == kExitPrevious) {
        ReturnToPlayerCount();
    } else {
        MetMultiTipsBaseScreen::OnExitFinished();
    }
}

// NTSC-U/C: 0x0030d908, PAL: 0x00333480
MetMultiTips1Screen::~MetMultiTips1Screen() {
}

// NTSC-U/C: 0x0030d988, PAL: 0x00333528
MetMultiTips1Screen *MetMultiTips1Screen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiTips1Screen(pRenderer, nPriority);
}
