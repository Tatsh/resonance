#include "met/metfreqmakerbuttonsscreen.h"

#include "met/metbuttonlist.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfrontendstate.h"
#include "met/metscreentitlescreen.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "fm_buttons";
static const char *const kDirectory = "metagame/persona";
static const char *const kContainerName = "freq_maker_buttons";

// The nine buttons in ring order, and the prompts their labels are read under.
static const char *const kRandomizeButton = "fm_RANDOMIZE.but";
static const char *const kBodyButton = "fm_BODY.but";
static const char *const kHeadButton = "fm_HEAD.but";
static const char *const kFaceButton = "fm_FACE.but";
static const char *const kDetailsButton = "fm_DETAILS.but";
static const char *const kLogosButton = "fm_LOGOS.but";
static const char *const kEditButton = "fm_EDIT.but";
static const char *const kNameButton = "fm_NAME.but";
static const char *const kSaveButton = "fm_SAVE.but";
static const char *const kRandomizePrompt = "fqmak_randomize";
static const char *const kBodyPrompt = "fqmak_body";
static const char *const kHeadPrompt = "fqmak_head";
static const char *const kFacePrompt = "fqmak_face";
static const char *const kDetailsPrompt = "fqmak_details";
static const char *const kLogosPrompt = "fqmak_logos";
static const char *const kEditPrompt = "fqmak_edit";
static const char *const kNamePrompt = "fqmak_name";
static const char *const kSavePrompt = "fqmak_save";

// The screen title keys and the save button's two labels.
static const char *const kCreateTitleKey = "freq_maker_create_buttons";
static const char *const kEditTitleKey = "freq_maker_edit_buttons";
static const char *const kSaveLabel = "SAVE";
static const char *const kDoneLabel = "DONE";

// The randomise button's label text and its two values.
static const char *const kRandomizeText = "RANDOMIZE.txt";
static const char *const kMutateLabel = "MUTATE";
static const char *const kRandomizeLabel = "RANDOMIZE";

static const char *const kCanvasScreen = "MetFreqMakerCanvasScreen";
static const char *const kButtonsScreen = "MetFreqMakerButtonsScreen";
static const char *const kDirectionsScreen = "MetFreqMakerDirectionsScreen";
static const char *const kInventoryScreen = "MetFreqMakerInventoryScreen";

// Configuration codes the button labels and the screen title are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The values of mEditing.
constexpr int kCreating = 0;
constexpr int kEditing = 1;

// The value of mUnknown9c while no action is pending.
constexpr int kNoPendingAction = -1;

// The button EnterAndShow() selects.
constexpr int kFirstButtonIndex = 0;

// The Rnd::Button states slot 30 sets. The names are inferred.
constexpr int kButtonStatePressed = 2;
constexpr int kButtonStateDisabled = 3;

// Add one button labelled from configuration. Slot 38 expands it for each button.
inline void AddButton(MetButtonList *pList, const char *pszObjectName, const char *pszPrompt) {
    HxStr objectName(pszObjectName);
    HxStr label;
    QueryConfigString(&label, kPromptConfigCode, pszPrompt);
    pList->Add(objectName, label);
}

} // namespace

// 0x00257968
MetFreqMakerButtonsScreen::MetFreqMakerButtonsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr), mUnknown9c(kNoPendingAction) {
    mButtonList = new MetButtonList();
    mEditing = kEditing;
}

// 0x0025e108
MetFreqMakerButtonsScreen::~MetFreqMakerButtonsScreen() {
    delete mButtonList;
}

// 0x0025e080
MetFreqMakerButtonsScreen *MetFreqMakerButtonsScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetFreqMakerButtonsScreen(pRenderer, nPriority);
}

// 0x00258c80
void MetFreqMakerButtonsScreen::EnterAndShow() {
    mButtonList->SetSelected(kFirstButtonIndex);
    if (mEditing == kCreating) {
        HxStr title;
        QueryConfigString(&title, kTitleConfigCode, kCreateTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    } else if (mEditing == kEditing) {
        HxStr title;
        QueryConfigString(&title, kTitleConfigCode, kEditTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    }
    Rnd::Button *pSave = dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(kSaveButton)));
    if (MetFrontEndState::shared()->mUnknown0c != 0) {
        pSave->mText->SetText(HxStr(kSaveLabel));
    } else {
        pSave->mText->SetText(HxStr(kDoneLabel));
    }
    UpdateRandomizeLabel();
    MetScreen::EnterAndShow();
}

// 0x00259fa0
int MetFreqMakerButtonsScreen::PollContainerLoad() {
    bool loaded = MetFreqMakerAssetManager::shared()->PollLoad();
    loaded = (FindScreenByName(HxStr(kDirectionsScreen))->PollContainerLoad() != 0) && loaded;
    loaded = (FindScreenByName(HxStr(kInventoryScreen))->PollContainerLoad() != 0) && loaded;
    loaded = loaded & (MetScreen::PollContainerLoad() != 0);
    if (!loaded) {
        return 0;
    }
    return MetScreen::PollContainerLoad(); // Yes, the binary polls the base load a second time.
}

// 0x0025e070
void MetFreqMakerButtonsScreen::PlayCycleLeftSound(int) {
}

// 0x0025e078
void MetFreqMakerButtonsScreen::PlayCycleRightSound(int) {
}

// 0x00258f40
void MetFreqMakerButtonsScreen::OnUnknownSlot30(Rnd::Object *) {
    ActivateNamedPanel(HxStr(kInventoryScreen));
    int nCount = mButtonList->mButtons.size();
    for (int i = 0; i < nCount; ++i) {
        mButtonList->ButtonAt(i)->SetState(kButtonStateDisabled);
    }
    mButtonList->mUnknown00->SetState(kButtonStatePressed);
}

// 0x00257b70
void MetFreqMakerButtonsScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    AddButton(mButtonList, kRandomizeButton, kRandomizePrompt);
    AddButton(mButtonList, kBodyButton, kBodyPrompt);
    AddButton(mButtonList, kHeadButton, kHeadPrompt);
    AddButton(mButtonList, kFaceButton, kFacePrompt);
    AddButton(mButtonList, kDetailsButton, kDetailsPrompt);
    AddButton(mButtonList, kLogosButton, kLogosPrompt);
    AddButton(mButtonList, kEditButton, kEditPrompt);
    AddButton(mButtonList, kNameButton, kNamePrompt);
    AddButton(mButtonList, kSaveButton, kSavePrompt);
}

// 0x0025a108
void MetFreqMakerButtonsScreen::OnUnknownSlot2(const HxStr &text) {
    if (text.mLen == 0) {
        return;
    }
    static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kCanvasScreen)))
        ->SetFreqName(text);
    ActivateNamedPanel(HxStr(kButtonsScreen));
}

// 0x0025e288
void MetFreqMakerButtonsScreen::SetEditing(int nEditing) {
    mEditing = nEditing;
    if (PollContainerLoad()) {
        UpdateRandomizeLabel();
    }
}

// 0x0025a228
void MetFreqMakerButtonsScreen::UpdateRandomizeLabel() {
    Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kRandomizeText)));
    switch (mEditing) {
    case kEditing:
        pText->SetText(HxStr(kMutateLabel));
        break;
    case kCreating:
        pText->SetText(HxStr(kRandomizeLabel));
        break;
    default:
        break;
    }
}
