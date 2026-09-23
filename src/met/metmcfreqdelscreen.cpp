#include "met/metmcfreqdelscreen.h"

#include <vector>

#include "app/playsound.h"
#include "game/freqappearance.h"
#include "memcard/memcardmanager.h"
#include "met/methelpscreen.h"
#include "met/metmsgscreen.h"
#include "met/metpersonadata.h"
#include "met/metpersonasaverscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "met/scrollinglist.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcfl";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_freq_load";
// The help text key.
static const char *const kHelpKey = "del_freq";

// Container objects.
static const char *const kListTitleText = "mcfl_listpan_title.txt";
static const char *const kDetailTitleText = "mcfl_freqpan_title.txt";
static const char *const kNameText = "mcfl_name.txt";
static const char *const kFaceMat = "mcfl_face.mat";
static const char *const kFreqMesh = "mcfl_freq01.mesh";
static const char *const kBirthdayText = "mcfl_dob.txt";
static const char *const kInfoText = "mcfl_info.txt";
static const char *const kRowView = "mcfl_line.view";
static const char *const kHighlightMesh = "mcfl_hilite.mesh";
static const char *const kUpArrowMesh = "mcfl_up.mesh";
static const char *const kDownArrowMesh = "mcfl_down.mesh";

// Panel texts, the list title format, and the help preset.
static const char *const kListTitleKey = "mcrf_freq";
static const char *const kDetailTitleKey = "mcrf_freqdata";
static const char *const kListTitleFormatKey = "mem_del_type";
static const char *const kOnlyBackPreset = "only_back_title";

// Dialogue names, keys, titles, and button labels.
static const char *const kLoadMessage = "freq_load";
static const char *const kLoadKey = "mem_load";
static const char *const kDeleteMessage = "okDelete";
static const char *const kDeleteKey = "freq_del_ok";
static const char *const kCopyMessage = "okCopy";
static const char *const kCopyKey = "freq_copy_ok";
static const char *const kLoadFailedMessage = "notifyloadfailed";
static const char *const kNoCardKey = "mc_load_fail_no_card";
static const char *const kLoadFailKey = "mc_load_fail";
static const char *const kNoFreqMessage = "no_freq_on_card";
static const char *const kWarningTitle = "WARNING";
static const char *const kConfirmTitle = "CONFIRM";
static const char *const kErrorTitle = "ERROR";
static const char *const kNoButton = "NO";
static const char *const kYesButton = "YES";
static const char *const kOkButton = "OK";

// The sound a copy or a deletion command plays.
static const char *const kToggleSound = "SND_MET_FM_TOGGLE";

static const char *const kOwnScreenName = "MetMCFreqDelScreen";
static const char *const kMemCardTypeScreen = "MetMemCardTypeScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kMsgScreen = "MetMsgScreen";
static const char *const kNoName = "";

// Configuration codes the dialogue texts and the titles are read under.
constexpr int kDialogueConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// Button counts MetMsgScreen receives with each dialogue.
constexpr int kNoButtons = 0;
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;

// The dialogue button OnMsgScreenDismissed() tests, counted from zero.
constexpr int kChoiceYes = 1;

// The two commands beyond MetScreenCommandCode the screen acts on.
constexpr int kCommandCopy = 7;
constexpr int kCommandDelete = 8;

// What MetScreen::mUnknown18 records for slot 36 to act on.
constexpr int kExitBack = 0;
constexpr int kExitToCopy = 1;
constexpr int kExitToDelete = 2;

// The list geometry.
constexpr int kRowPitch = 40;
constexpr int kVisibleRows = 8;
constexpr int kListContext = 0;

// The persona burn slot and the material stage that shows it.
constexpr int kBurnSlot = 0;
constexpr int kBurnStage = 1;

// The two StartSave() flags a copy passes.
constexpr int kCopySaveFlag9c = 1;
constexpr int kCopySaveFlag98 = 1;

// The connect status of a present card.
constexpr int kCardPresent = 0;

// The load results OnPersonasLoaded() treats as a loaded card. The meanings of 3 and 11 are not
// recovered.
constexpr int kLoadStatusLoaded = 0;
constexpr int kLoadStatusAccepted3 = 3;
constexpr int kLoadStatusAccepted11 = 11;

// The text a row past the end of the list shows.
static const char *const kNoText = "";

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A configuration value read by value.
inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr value = QueryConfigString(nCode, pszKey);
    return value;
}

// Resolve one named object of the renderer as T.
template <class T>
inline T *FindObject(const char *pszName) {
    return dynamic_cast<T *>(Rnd::g_manager.Find(HxStr(pszName)));
}

// Delete every persona of a list and empty it.
inline void DeletePersonas(std::vector<MetPersonaData *> &personas) {
    for (unsigned int i = 0; i < personas.size(); ++i) {
        delete personas[i];
    }
    personas.clear();
}

} // namespace

// 0x002be968
MetMCFreqDelScreen::MetMCFreqDelScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mDeleting(0), mList(nullptr), mCopyPersona(nullptr), mLoadPending(0) {
    mUnknown60 = 0;
    mUnknown38.push_back(HxStr(kHelpKey));
}

// 0x002beca8
MetMCFreqDelScreen::~MetMCFreqDelScreen() {
    if (mList != nullptr) {
        delete mList;
        mList = nullptr;
    }
    DeletePersonas(mPersonas);
}

// 0x002c5b60
MetMCFreqDelScreen *MetMCFreqDelScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMCFreqDelScreen(pRenderer, nPriority);
}

// 0x002bee90
void MetMCFreqDelScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    Rnd::Text *pListTitle = FindObject<Rnd::Text>(kListTitleText);
    pListTitle->SetText(ConfigText(kDialogueConfigCode, kListTitleKey));
    pListTitle->SetShowing(1);

    Rnd::Text *pDetailTitle = FindObject<Rnd::Text>(kDetailTitleText);
    pDetailTitle->SetText(ConfigText(kDialogueConfigCode, kDetailTitleKey));
    pDetailTitle->SetShowing(1);

    mNameText = FindObject<Rnd::Text>(kNameText);
    mFaceMat = FindObject<Rnd::Mat>(kFaceMat);
    mBurnTexture = FreqAppearance::FindPersonaBurnTexture(kBurnSlot);
    mFreqMesh = FindObject<Rnd::Mesh>(kFreqMesh);
    mBirthdayText = FindObject<Rnd::Text>(kBirthdayText);
}

// 0x002bf3a8
void MetMCFreqDelScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        if (mList->getSelected() > 0) {
            mList->scrollUp();
            ShowSelection();
        }
        break;

    case kMetScreenCommandNext:
        if (static_cast<unsigned int>(mList->getSelected()) < mPersonas.size() - 1) {
            mList->scrollDown();
            ShowSelection();
        }
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    case kCommandCopy:
        if (mPersonas.size() == 0) {
            break;
        }
        PlaySoundByName(kToggleSound);
        mUnknown18 = kExitToCopy;
        mCopyPersona = mPersonas[mList->getSelected()];
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        BeginExit();
        break;

    case kCommandDelete:
        if (mPersonas.size() == 0) {
            break;
        }
        PlaySoundByName(kToggleSound);
        mUnknown18 = kExitToDelete;
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x002bf750
void MetMCFreqDelScreen::OnUnknownSlot7() {
    if (!mLoadPending) {
        return;
    }
    std::vector<HxStr> buttons;
    const HxStr format(ConfigText(kDialogueConfigCode, kLoadKey));
    const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
    MetMsgScreen::Show(HxStr(kLoadMessage), HxStr(kWarningTitle), text, kNoButtons, buttons, this);
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateGetConnectStateTask(mCardSlot.mPortSlot);
}

// 0x002bfa88
void MetMCFreqDelScreen::ShowList() {
    if (mList == nullptr) {
        Rnd::View *pRow = FindObject<Rnd::View>(kRowView);
        Rnd::Mesh *pHighlight = FindObject<Rnd::Mesh>(kHighlightMesh);
        Rnd::Mesh *pUpArrow = FindObject<Rnd::Mesh>(kUpArrowMesh);
        Rnd::Mesh *pDownArrow = FindObject<Rnd::Mesh>(kDownArrowMesh);
        mList = new ScrollingList(
            this, kRowPitch, kVisibleRows, pRow, pHighlight, pUpArrow, pDownArrow, kListContext);
    }
    mList->setShowing(1);
    mList->setItemCount(mPersonas.size());
    mList->setSelected(0);
    mList->refresh();
    ShowSelection();

    const HxStr format(ConfigText(kTitleConfigCode, kListTitleFormatKey));
    MetScreenTitleScreen::SetTitle(
        HxStr(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName))));
    PushNamedScreen(HxStr(kHelpScreen));
    MetHelpScreen::SelectPreset(HxStr(kOnlyBackPreset));
    MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
    MetScreen::EnterAndShow();
    mLoadPending = 0;
}

// 0x002bff98
void MetMCFreqDelScreen::ShowSelection() {
    Rnd::Text *pInfo = FindObject<Rnd::Text>(kInfoText);
    if (mPersonas.size() != 0) {
        MetPersonaData *pPersona = mPersonas[mList->getSelected()];
        mNameText->SetShowing(1);
        mNameText->SetText(pPersona->mUnknown140.mUnknown00);
        mFreqMesh->SetShowing(1);
        pPersona->AttachToBurnSlot(kBurnSlot);
        mFaceMat->mStages[kBurnStage].SetTex(mBurnTexture);
        mBirthdayText->SetShowing(1);
        mBirthdayText->SetText(pPersona->mUnknown154);
        pInfo->SetShowing(1);
    } else {
        mNameText->SetShowing(0);
        mFreqMesh->SetShowing(0);
        mBirthdayText->SetShowing(0);
        pInfo->SetShowing(0);
    }
}

// 0x002c01c0
void MetMCFreqDelScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitToDelete) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        MetMsgScreen::Show(HxStr(kDeleteMessage),
                           HxStr(kConfirmTitle),
                           ConfigText(kDialogueConfigCode, kDeleteKey),
                           kTwoButtons,
                           buttons,
                           this);
    } else if (mUnknown18 == kExitToCopy) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        const HxStr format(ConfigText(kDialogueConfigCode, kCopyKey));
        const MemcardConnectState next(NextCardSlot(mCardSlot));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(next.mSlotName)));
        MetMsgScreen::Show(
            HxStr(kCopyMessage), HxStr(kConfirmTitle), text, kTwoButtons, buttons, this);
    } else {
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kMemCardTypeScreen));
        ActivateNamedPanel(HxStr(kMemCardTypeScreen));
        mList->setShowing(0);
    }
}

// 0x002c0ad8
void MetMCFreqDelScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kDeleteMessage) {
        if (nChoice == kChoiceYes) {
            mDeleting = nChoice;
            std::vector<HxStr> screens;
            screens.push_back(HxStr(kOwnScreenName));
            MetPersonaSaverScreen::StartDelete(screens, mPersonas[mList->getSelected()], mCardSlot);
        } else {
            PushNamedScreen(HxStr(kOwnScreenName));
            ActivateNamedPanel(HxStr(kOwnScreenName));
        }
    } else if (name == kCopyMessage) {
        if (nChoice == kChoiceYes) {
            const MemcardConnectState next(NextCardSlot(mCardSlot));
            std::vector<HxStr> screens;
            screens.push_back(HxStr(kOwnScreenName));
            MetPersonaSaverScreen::StartSave(
                screens, mCopyPersona, next, kCopySaveFlag9c, kCopySaveFlag98);
            mCopyPersona = nullptr;
        } else {
            PushNamedScreen(HxStr(kOwnScreenName));
            ActivateNamedPanel(HxStr(kOwnScreenName));
        }
    } else if (name == kLoadFailedMessage || name == kNoFreqMessage) {
        mUnknown10->RemoveScreen(this);
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kMemCardTypeScreen));
        ActivateNamedPanel(HxStr(kMemCardTypeScreen));
    } else {
        mLoadPending = 0;
        ShowList();
        ActivateNamedPanel(HxStr(kOwnScreenName));
    }
}

// 0x002c1510
void MetMCFreqDelScreen::OnPersonasLoaded(int, int nStatus) {
    if (nStatus != kLoadStatusLoaded && nStatus != kLoadStatusAccepted3 &&
        nStatus != kLoadStatusAccepted11) {
        const HxStr format(ConfigText(kDialogueConfigCode, kLoadFailKey));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kOkButton));
        MetMsgScreen::Show(
            HxStr(kLoadFailedMessage), HxStr(kErrorTitle), text, kOneButton, buttons, this);
        return;
    }

    if (!mLoadPending) {
        return;
    }
    if (mPersonas.size() != 0 || mDeleting) {
        mDeleting = 0;
        ExitScreenByName(HxStr(kMsgScreen));
        return;
    }

    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kOkButton));
    const HxStr format(ConfigText(kDialogueConfigCode, kNoFreqMessage));
    const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
    MetMsgScreen::Show(HxStr(kNoFreqMessage), HxStr(kErrorTitle), text, kOneButton, buttons, this);
}

// 0x002c1d10
void MetMCFreqDelScreen::OnConnectState(MemcardConnectState, int nStatus) {
    if (nStatus == kCardPresent) {
        MemcardManager::shared()->mUser = this;
        DeletePersonas(mPersonas);
        MemcardManager::shared()->CreateLoadPersonasTask(mCardSlot.mPortSlot, &mPersonas);
        return;
    }

    const HxStr format(ConfigText(kDialogueConfigCode, kNoCardKey));
    const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kOkButton));
    MetMsgScreen::Show(
        HxStr(kLoadFailedMessage), HxStr(kErrorTitle), text, kOneButton, buttons, this);
}

// 0x002c5b40
int MetMCFreqDelScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}

// 0x002c5be8
void MetMCFreqDelScreen::EnterAndShow() {
    SetShowing(0);
    mLoadPending = 1;
}

// 0x002c5c28
int MetMCFreqDelScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    if (static_cast<unsigned>(nItem) < mPersonas.size()) {
        pText->SetText(mPersonas[nItem]->mUnknown140.mUnknown00);
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

// 0x002c5d10
void MetMCFreqDelScreen::SetCardSlot(MemcardConnectState slot) {
    mCardSlot = slot;
    mLoadPending = 0;
}
