#include "met/metmemcardtypescreen.h"

#include <vector>

#include "met/metbuttonlist.h"
#include "met/methelpscreen.h"
#include "met/metmcfreqdelscreen.h"
#include "met/metremixdelscreen.h"
#include "met/metremixmanager.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcrf";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "mcrf_load";

// The two buttons, and the keys of their labels and help texts.
static const char *const kRemixButton = "mcrf_01.but";
static const char *const kFreqButton = "mcrf_02.but";
static const char *const kRemixKey = "mcrf_remix";
static const char *const kFreqKey = "mcrf_freq";

// The title format, filled with the card's name, and the help preset.
static const char *const kTitleFormatKey = "mem_del_type";
static const char *const kStandardTitlePreset = "standard_title";

static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kMemCardLoadScreen = "MetMemCardLoadScreen";
static const char *const kRemixDelScreen = "MetRemixDelScreen";
static const char *const kMCFreqDelScreen = "MetMCFreqDelScreen";
static const char *const kNoName = "";

// Configuration codes the button labels and the title format are read under.
constexpr int kLabelConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The button ring indices.
constexpr int kNoSelection = -1;
constexpr int kRemixButtonIndex = 0;

// What MetScreen::mUnknown18 records for slot 36 to act on.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The alternation the select command starts on the chosen button.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// The one return screen ListRemixes() receives, and the playlist flag it passes.
constexpr int kRemixReturnScreenCount = 1;
constexpr int kNoPlayList = 0;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Add one button labelled from configuration. Slot 38 expands it for each button.
inline void AddButton(MetButtonList *pList, const char *pszObjectName, const char *pszLabelKey) {
    HxStr objectName(pszObjectName);
    HxStr label = QueryConfigString(kLabelConfigCode, pszLabelKey);
    pList->Add(objectName, label);
}

} // namespace

// 0x002d23b8
MetMemCardTypeScreen::MetMemCardTypeScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr) {
    mUnknown38.push_back(HxStr(kRemixKey));
    mUnknown38.push_back(HxStr(kFreqKey));
}

// 0x002d84d0
MetMemCardTypeScreen::~MetMemCardTypeScreen() {
    delete mButtonList;
}

// 0x002d8448
MetMemCardTypeScreen *MetMemCardTypeScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMemCardTypeScreen(pRenderer, nPriority);
}

// 0x002d8560
void MetMemCardTypeScreen::SetCardSlot(MemcardConnectState slot) {
    mCardSlot = slot;
}

// 0x002d2b50
void MetMemCardTypeScreen::EnterAndShow() {
    if (mButtonList->mSelected == kNoSelection) {
        mButtonList->SetSelected(kRemixButtonIndex);
    }
    HxStr format = QueryConfigString(kTitleConfigCode, kTitleFormatKey);
    const HxStr title(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
    MetScreenTitleScreen::SetTitle(title);
    MetHelpScreen::SelectPreset(HxStr(kStandardTitlePreset));
    MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
    MetScreen::EnterAndShow();
}

// 0x002d2898
void MetMemCardTypeScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtonList->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mButtonList->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mButtonList->mUnknown00,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x002d2cf0
void MetMemCardTypeScreen::OnUnknownSlot30(Rnd::Button *) {
    mUnknown18 = kExitToButtonAction;
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}

// 0x002d2e90
void MetMemCardTypeScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitBack) {
        PushNamedScreen(HxStr(kMemCardLoadScreen));
        ActivateNamedPanel(HxStr(kMemCardLoadScreen));
        return;
    }

    if (mButtonList->mSelected == kRemixButtonIndex) {
        std::vector<HxStr> screens;
        screens.resize(kRemixReturnScreenCount);
        screens[0] = kRemixDelScreen;
        std::vector<MemcardConnectState> slots;
        slots.push_back(mCardSlot);
        static_cast<MetRemixDelScreen *>(FindScreenByName(HxStr(kRemixDelScreen)))
            ->SetCardSlot(mCardSlot);
        MetRemixManager::shared()->ListRemixes(screens, slots, kNoPlayList);
    } else {
        static_cast<MetMCFreqDelScreen *>(FindScreenByName(HxStr(kMCFreqDelScreen)))
            ->SetCardSlot(mCardSlot);
        PushNamedScreen(HxStr(kMCFreqDelScreen));
        ActivateNamedPanel(HxStr(kMCFreqDelScreen));
    }
}

// 0x002d26b0
void MetMemCardTypeScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mButtonList = new MetButtonList;
    AddButton(mButtonList, kRemixButton, kRemixKey);
    AddButton(mButtonList, kFreqButton, kFreqKey);
    mButtonList->SetSelected(kNoSelection);
}
