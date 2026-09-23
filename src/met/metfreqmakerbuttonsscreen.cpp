#include "met/metfreqmakerbuttonsscreen.h"

#include <cstdio>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/globalsettings.h"
#include "memcard/memcardconnectstate.h"
#include "met/metbuttonlist.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfreqmakerdirectionsscreen.h"
#include "met/metfreqmakerinventoryscreen.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metkeyboardrequest.h"
#include "met/metkeyboardscreen.h"
#include "met/metmsgscreen.h"
#include "met/metpersonadata.h"
#include "met/metpersonasaverscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/r250.h"
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

// The values of mExitAction.
constexpr int kExitNone = -1;
constexpr int kExitBack = 0;
constexpr int kExitSave = 1;

// The MetScreen::mUnknown18 value slot 19 writes for every exit.
constexpr int kExitUnknown18 = 0;

static const char *const kKeyboardScreen = "MetKeyboardScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kModeScreen = "MetModeScreen";
static const char *const kNetPortalScreen = "MetNetPortalScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kNoName = "";

// The keyboard request the name button builds.
static const char *const kNamePromptText = "FreQ name";
static const char *const kNameTicker = "met_fm_name_freq";
constexpr int kAnyPad = -1;
constexpr int kNameMaxWidth = 176;
constexpr int kNameMaxLength = 12;

// The save-before-leaving dialogue.
static const char *const kCheckChangedDialogue = "check_if_changed";
static const char *const kWarningTitle = "WARNING";
static const char *const kCheckChangedFormat = "%s has been edited.  Do you wish to save?";
static const char *const kYesButton = "YES";
static const char *const kNoButton = "NO";
constexpr int kTwoButtons = 2;
constexpr int kChoiceYes = 0;
constexpr int kChoiceNo = 1;

// The stand-in card slot a save without a card uses.
static const char *const kStandInSlotName = "1";
constexpr int kStandInPortSlot = 0;

// The screens a save returns to, and their count.
constexpr int kSaveReturnScreenCount = 3;

// The alternation the select command starts on the chosen button.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// Rounds the random pre-fab index to the nearest.
constexpr double kRoundHalf = 0.5;

// The MetFreqMakerDirectionsScreen pages the row shows.
enum DirectionsPage {
    kDirectionsRandomize = 4,
    kDirectionsMutate = 5,
    kDirectionsBody = 6,
    kDirectionsHead = 7,
    kDirectionsFace = 8,
    kDirectionsDetails = 9,
    kDirectionsLogos = 10,
    kDirectionsEdit = 11,
    kDirectionsName = 12,
    kDirectionsSave = 13,
    kDirectionsAccept = 14,
    kDirectionsBlank = 16
};

// The Rnd::Button state slot 7 gives every button and the one it gives the selected button.
constexpr int kButtonStateNormal = 0;
constexpr int kButtonStateSelected = 1;

// The button EnterAndShow() selects.
constexpr int kFirstButtonIndex = 0;

// The Rnd::Button states slot 30 sets. The names are inferred.
constexpr int kButtonStatePressed = 2;
constexpr int kButtonStateDisabled = 3;

// The stack buffer the save-before-leaving text is formatted into.
constexpr int kCheckChangedBufferSize = 112;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

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
      mButtonList(nullptr), mExitAction(kExitNone) {
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

// 0x0025e1e0
void MetFreqMakerButtonsScreen::OnUnknownSlot7() {
    ShowPageForButton(mButtonList->mUnknown00);
    int nCount = mButtonList->mButtons.size();
    for (int i = 0; i < nCount; ++i) {
        mButtonList->ButtonAt(i)->SetState(kButtonStateNormal);
    }
    mButtonList->mUnknown00->SetState(kButtonStateSelected);
    ShowDirectionsForButton(mButtonList->mUnknown00);
}

// Expanded into slots 15 and 36.
inline void MetFreqMakerButtonsScreen::StartPersonaSave(const std::vector<HxStr> &screens) {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kCanvasScreen)));
    int nNewPersona = mNewPersona == 1;
    if (MetFrontEndState::shared()->mUnknown0c != 0) {
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        MetPersonaSaverScreen::StartSave(
            screens, pCanvas->mPersona, GlobalSettings::shared()->mCardSlots[0], nNewPersona, 0);
    } else {
        MemcardConnectState slot;
        slot.mSlotName = kStandInSlotName;
        slot.mPortSlot = kStandInPortSlot;
        MetPersonaSaverScreen::StartSave(screens, pCanvas->mPersona, slot, nNewPersona, 0);
    }
}

// 0x00259ad0
void MetFreqMakerButtonsScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (!(name == kCheckChangedDialogue)) {
        return;
    }
    switch (nChoice) {
    case kChoiceYes: {
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kCanvasScreen)))
            ->CommitPersona();
        std::vector<HxStr> screens;
        screens.push_back(MetFrontEndState::shared()->mUnknown24);
        screens.push_back(HxStr(kHelpScreen));
        StartPersonaSave(screens);
        break;
    }

    case kChoiceNo:
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(MetFrontEndState::shared()->mUnknown24);
        ActivateNamedPanel(MetFrontEndState::shared()->mUnknown24);
        break;

    default:
        break;
    }
}

// 0x002581c8
void MetFreqMakerButtonsScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtonList->OnUnknownSlot2();
        ShowPageForButton(mButtonList->mUnknown00);
        ShowDirectionsForButton(mButtonList->mUnknown00);
        break;

    case kMetScreenCommandNext:
        mButtonList->OnUnknownSlot3();
        ShowPageForButton(mButtonList->mUnknown00);
        ShowDirectionsForButton(mButtonList->mUnknown00);
        break;

    case kMetScreenCommandSelect: {
        const HxStr &selected = mButtonList->mUnknown00->mName;
        if (selected == kNameButton) {
            HxStr *pName =
                static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kCanvasScreen)))
                    ->GetFreqName();
            static_cast<MetKeyboardScreen *>(FindScreenByName(HxStr(kKeyboardScreen)))
                ->ResetKeyStates();
            MetKeyboardRequest request(
                HxStr(kButtonsScreen), HxStr(kNamePromptText), *pName, kAnyPad, this);
            request.mMaxLength = kNameMaxLength;
            request.mMaxWidth = kNameMaxWidth;
            request.mTicker = kNameTicker;
            MetKeyboardScreen::Open(request);
        } else if (selected == kSaveButton) {
            static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kCanvasScreen)))
                ->CommitPersona();
            mExitAction = kExitSave;
            ActivateNamedPanel(HxStr(kNoName));
            mUnknown18 = kExitUnknown18;
            ExitScreenByName(HxStr(kHelpScreen));
            ExitScreenByName(HxStr(kTitleScreen));
            ExitScreenByName(HxStr(kCanvasScreen));
            ExitScreenByName(HxStr(kDirectionsScreen));
            ExitScreenByName(HxStr(kInventoryScreen));
            BeginExit();
        } else if (selected == kRandomizeButton) {
            MetFreqMakerCanvasScreen *pCanvas =
                static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kCanvasScreen)));
            if (mEditing == kCreating) {
                std::vector<MetPersonaData *> *pPrefabs =
                    MetFreqMakerAssetManager::shared()->GetIdentityList();
                int nLast = pPrefabs->size() - 1;
                int nIndex =
                    static_cast<int>(static_cast<double>(RandomFloat() * nLast) + kRoundHalf);
                pCanvas->LoadPrefab((*pPrefabs)[nIndex], 1);
            } else if (mEditing == kEditing) {
                pCanvas->Randomize();
            }
        } else {
            ActivateNamedPanel(HxStr(kNoName));
            StartRepeatingSound(mUnknown10->mUnknown68,
                                kSelectAlternateInterval,
                                mButtonList->mUnknown00,
                                kSelectAlternateCycles);
        }
        break;
    }

    case kMetScreenCommandBack:
        mExitAction = kExitBack;
        ActivateNamedPanel(HxStr(kNoName));
        mUnknown18 = kExitUnknown18;
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kCanvasScreen));
        ExitScreenByName(HxStr(kDirectionsScreen));
        ExitScreenByName(HxStr(kInventoryScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x0025e1a0
void MetFreqMakerButtonsScreen::OnUnknownSlot33() {
    ShowPageForButton(mButtonList->mUnknown00);
    ShowDirectionsForButton(mButtonList->mUnknown00);
}

// 0x00259040
void MetFreqMakerButtonsScreen::OnUnknownSlot36() {
    static_cast<MetFreqMakerInventoryScreen *>(FindScreenByName(HxStr(kInventoryScreen)))
        ->HidePages();
    switch (mExitAction) {
    case kExitSave: {
        std::vector<HxStr> screens;
        if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeNet) {
            screens.resize(kSaveReturnScreenCount);
            screens[0] = kNetPortalScreen;
        } else {
            screens.resize(kSaveReturnScreenCount);
            screens[0] = kModeScreen;
        }
        screens[1] = kLeftGizmoScreen;
        screens[2] = kHelpScreen;
        StartPersonaSave(screens);
        break;
    }

    case kExitBack: {
        MetFreqMakerCanvasScreen *pCanvas =
            static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kCanvasScreen)));
        if (MetFrontEndState::shared()->mUnknown0c != 0 && pCanvas->mModified != 0) {
            std::vector<HxStr> buttons;
            buttons.push_back(HxStr(kYesButton));
            buttons.push_back(HxStr(kNoButton));
            const HxStr freqName(*pCanvas->GetFreqName());
            char szText[kCheckChangedBufferSize];
            sprintf(szText, kCheckChangedFormat, TextOrEmpty(freqName));
            MetMsgScreen::Show(HxStr(kCheckChangedDialogue),
                               HxStr(kWarningTitle),
                               HxStr(szText),
                               kTwoButtons,
                               buttons,
                               this);
            MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        } else {
            PushNamedScreen(MetFrontEndState::shared()->mUnknown24);
            PushNamedScreen(HxStr(kHelpScreen));
            ActivateNamedPanel(MetFrontEndState::shared()->mUnknown24);
        }
        break;
    }

    default:
        break;
    }
}

// 0x0025a3e0
void MetFreqMakerButtonsScreen::ShowPageForButton(Rnd::Button *pButton) {
    MetFreqMakerInventoryScreen *pInventory =
        static_cast<MetFreqMakerInventoryScreen *>(FindScreenByName(HxStr(kInventoryScreen)));
    FindScreenByName(HxStr(kDirectionsScreen)); // Yes, the binary discards this lookup.
    const HxStr &name = pButton->mName;
    if (name == kBodyButton) {
        pInventory->ShowBodyPage();
    } else if (name == kHeadButton) {
        pInventory->ShowHeadPage();
    } else if (name == kFaceButton) {
        pInventory->ShowFacePage();
    } else if (name == kDetailsButton) {
        pInventory->ShowDetailsPage();
    } else if (name == kLogosButton) {
        pInventory->ShowLogosPage();
    } else if (name == kEditButton) {
        pInventory->ShowEditPage();
    } else if (name == kNameButton || name == kRandomizeButton || name == kSaveButton) {
        pInventory->HidePages();
    }
}

// 0x0025a5e0
void MetFreqMakerButtonsScreen::ShowDirectionsForButton(Rnd::Button *pButton) {
    MetFreqMakerDirectionsScreen *pDirections =
        static_cast<MetFreqMakerDirectionsScreen *>(FindScreenByName(HxStr(kDirectionsScreen)));
    if (pButton == nullptr) {
        pDirections->ShowPage(kDirectionsBlank);
        return;
    }
    const HxStr &name = pButton->mName;
    if (name == kBodyButton) {
        pDirections->ShowPage(kDirectionsBody);
    } else if (name == kHeadButton) {
        pDirections->ShowPage(kDirectionsHead);
    } else if (name == kFaceButton) {
        pDirections->ShowPage(kDirectionsFace);
    } else if (name == kDetailsButton) {
        pDirections->ShowPage(kDirectionsDetails);
    } else if (name == kLogosButton) {
        pDirections->ShowPage(kDirectionsLogos);
    } else if (name == kEditButton) {
        pDirections->ShowPage(kDirectionsEdit);
    } else if (name == kNameButton) {
        pDirections->ShowPage(kDirectionsName);
    } else if (name == kRandomizeButton) {
        if (mEditing == kCreating) {
            pDirections->ShowPage(kDirectionsRandomize);
        } else if (mEditing == kEditing) {
            pDirections->ShowPage(kDirectionsMutate);
        }
    } else if (name == kSaveButton) {
        if (MetFrontEndState::shared()->mUnknown0c != 0) {
            pDirections->ShowPage(kDirectionsSave);
        } else {
            pDirections->ShowPage(kDirectionsAccept);
        }
    }
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
