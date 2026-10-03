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
#include "met/metstrings.h"
#include "met/scrollinglist.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "rnd/view.h"

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

// What MetScreen::mExitChoice records for slot 36 to act on.
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

// The nConfirmReplace and nIsCopy arguments a copy passes to StartSave().
constexpr int kCopyConfirmReplace = 1;
constexpr int kCopyIsCopy = 1;

// The connect status of a present card.
constexpr int kCardPresent = 0;

// The load results OnPersonasLoaded() treats as a loaded card. The values are those of
// kMemcardStatusOk, kMemcardStatusNoEntry, and kMemcardStatusNoFile.
constexpr int kLoadStatusLoaded = 0;
constexpr int kLoadStatusNoEntry = 3;
constexpr int kLoadStatusNoFile = 11;

// The text a row past the end of the list shows.
static const char *const kNoText = "";

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Resolve one named object of the renderer as T.
template <class T>
inline T *FindObject(const char *pszName) {
    return dynamic_cast<T *>(Rnd::TheManager.Find(HxStr(pszName)));
}

// Delete every persona of a list and empty it.
inline void DeletePersonas(std::vector<MetPersonaData *> &personas) {
    for (unsigned int i = 0; i < personas.size(); ++i) {
        delete personas[i];
    }
    personas.clear();
}

} // namespace

// NTSC-U/C: 0x002be968, PAL: 0x002de6a8
MetMCFreqDelScreen::MetMCFreqDelScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mDeleting(0), mList(nullptr), mCopyPersona(nullptr), mLoadPending(0) {
    mShowsLoadedDrawables = 0;
    mHelpKeys.push_back(MetText(kMetStrHDelFreq, kHelpKey));
}

// NTSC-U/C: 0x002beca8, PAL: 0x002dea70
MetMCFreqDelScreen::~MetMCFreqDelScreen() {
    if (mList != nullptr) {
        delete mList;
        mList = nullptr;
    }
    DeletePersonas(mPersonas);
}

// NTSC-U/C: 0x002c5b60, PAL: 0x002e62d0
MetMCFreqDelScreen *MetMCFreqDelScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMCFreqDelScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x002bee90, PAL: 0x002dec68
void MetMCFreqDelScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    Rnd::Text *pListTitle = FindObject<Rnd::Text>(kListTitleText);
    pListTitle->SetText(MetConfigText(kMetStrMcrfFreq, kDialogueConfigCode, kListTitleKey));
    pListTitle->SetShowing(1);

    Rnd::Text *pDetailTitle = FindObject<Rnd::Text>(kDetailTitleText);
    pDetailTitle->SetText(MetConfigText(kMetStrMcrfFreqdata, kDialogueConfigCode, kDetailTitleKey));
    pDetailTitle->SetShowing(1);

    mNameText = FindObject<Rnd::Text>(kNameText);
    mFaceMat = FindObject<Rnd::Mat>(kFaceMat);
    mBurnTexture = FreqAppearance::FindPersonaBurnTexture(kBurnSlot);
    mFreqMesh = FindObject<Rnd::Mesh>(kFreqMesh);
    mBirthdayText = FindObject<Rnd::Text>(kBirthdayText);
}

// NTSC-U/C: 0x002bf3a8, PAL: 0x002df258
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
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    case kCommandCopy:
        if (mPersonas.size() == 0) {
            break;
        }
        PlaySoundByName(kToggleSound);
        mExitChoice = kExitToCopy;
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
        mExitChoice = kExitToDelete;
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x002bf750, PAL: 0x002df6a8
void MetMCFreqDelScreen::OnPanelActivated() {
    if (!mLoadPending) {
        return;
    }
    std::vector<HxStr> buttons;
    const HxStr format(MetConfigText(kMetStrMemLoad, kDialogueConfigCode, kLoadKey));
    const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
    MetMsgScreen::Show(HxStr(kLoadMessage),
                       MetText(kMetStrMsgWARNING, kWarningTitle),
                       text,
                       kNoButtons,
                       buttons,
                       this);
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateGetConnectStateTask(mCardSlot.mPortSlot);
}

// NTSC-U/C: 0x002bfa88, PAL: 0x002dfa50
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

    const HxStr format(MetConfigText(kMetStrTMemDelType, kTitleConfigCode, kListTitleFormatKey));
    MetScreenTitleScreen::SetTitle(
        HxStr(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName))));
    PushNamedScreen(HxStr(kHelpScreen));
    MetHelpScreen::SelectPreset(MetText(kMetStrHOnlyBackTitle, kOnlyBackPreset));
    MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
    MetScreen::EnterAndShow();
    mLoadPending = 0;
}

// NTSC-U/C: 0x002bff98, PAL: 0x002e0030
void MetMCFreqDelScreen::ShowSelection() {
    Rnd::Text *pInfo = FindObject<Rnd::Text>(kInfoText);
#ifdef VIDEO_STANDARD_PAL
    pInfo->SetText(GetMetString(kMetStrMcrfBorn));
#endif
    if (mPersonas.size() != 0) {
        MetPersonaData *pPersona = mPersonas[mList->getSelected()];
        mNameText->SetShowing(1);
        mNameText->SetText(pPersona->mAppearance.mUserName);
        mFreqMesh->SetShowing(1);
        pPersona->AttachToBurnSlot(kBurnSlot);
        mFaceMat->mStages[kBurnStage].SetTex(mBurnTexture);
        mBirthdayText->SetShowing(1);
        mBirthdayText->SetText(pPersona->mBirthday);
        pInfo->SetShowing(1);
    } else {
        mNameText->SetShowing(0);
        mFreqMesh->SetShowing(0);
        mBirthdayText->SetShowing(0);
        pInfo->SetShowing(0);
    }
}

// NTSC-U/C: 0x002c01c0, PAL: 0x002e0310
void MetMCFreqDelScreen::OnExitFinished() {
    if (mExitChoice == kExitToDelete) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
        buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
        MetMsgScreen::Show(HxStr(kDeleteMessage),
                           MetText(kMetStrMsgCONFIRM, kConfirmTitle),
                           MetConfigText(kMetStrFreqDelOk, kDialogueConfigCode, kDeleteKey),
                           kTwoButtons,
                           buttons,
                           this);
    } else if (mExitChoice == kExitToCopy) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
        buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
        const HxStr format(MetConfigText(kMetStrFreqCopyOk, kDialogueConfigCode, kCopyKey));
        const MemcardConnectState next(NextCardSlot(mCardSlot));
        const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(next.mSlotName)));
        MetMsgScreen::Show(HxStr(kCopyMessage),
                           MetText(kMetStrMsgCONFIRM, kConfirmTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
    } else {
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kMemCardTypeScreen));
        ActivateNamedPanel(HxStr(kMemCardTypeScreen));
        mList->setShowing(0);
    }
}

// NTSC-U/C: 0x002c0ad8, PAL: 0x002e0dc0
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
                screens, mCopyPersona, next, kCopyConfirmReplace, kCopyIsCopy);
            mCopyPersona = nullptr;
        } else {
            PushNamedScreen(HxStr(kOwnScreenName));
            ActivateNamedPanel(HxStr(kOwnScreenName));
        }
    } else if (name == kLoadFailedMessage || name == kNoFreqMessage) {
        mRenderer->RemoveScreen(this);
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

// NTSC-U/C: 0x002c1510, PAL: 0x002e19f8
void MetMCFreqDelScreen::OnPersonasLoaded(int, int nStatus) {
    if (nStatus != kLoadStatusLoaded && nStatus != kLoadStatusNoEntry &&
        nStatus != kLoadStatusNoFile) {
        const HxStr format(MetConfigText(kMetStrMcLoadFail, kDialogueConfigCode, kLoadFailKey));
        const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
        MetMsgScreen::Show(HxStr(kLoadFailedMessage),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           text,
                           kOneButton,
                           buttons,
                           this);
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
    buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
    const HxStr format(MetConfigText(kMetStrNoFreqOnCard, kDialogueConfigCode, kNoFreqMessage));
    const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
    MetMsgScreen::Show(HxStr(kNoFreqMessage),
                       MetText(kMetStrMsgERROR, kErrorTitle),
                       text,
                       kOneButton,
                       buttons,
                       this);
}

// NTSC-U/C: 0x002c1d10, PAL: 0x002e2310
void MetMCFreqDelScreen::OnConnectState(MemcardConnectState, int nStatus) {
    if (nStatus == kCardPresent) {
        MemcardManager::shared()->mUser = this;
        DeletePersonas(mPersonas);
        MemcardManager::shared()->CreateLoadPersonasTask(mCardSlot.mPortSlot, &mPersonas);
        return;
    }

    const HxStr format(MetConfigText(kMetStrMcLoadFailNoCard, kDialogueConfigCode, kNoCardKey));
    const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
    MetMsgScreen::Show(HxStr(kLoadFailedMessage),
                       MetText(kMetStrMsgERROR, kErrorTitle),
                       text,
                       kOneButton,
                       buttons,
                       this);
}

// NTSC-U/C: 0x002c5b40, PAL: 0x002e62b0
int MetMCFreqDelScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}

// NTSC-U/C: 0x002c5be8, PAL: 0x002e6358
void MetMCFreqDelScreen::EnterAndShow() {
    SetShowing(0);
    mLoadPending = 1;
}

// NTSC-U/C: 0x002c5c28, PAL: 0x002e6398
int MetMCFreqDelScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    if (static_cast<unsigned>(nItem) < mPersonas.size()) {
        pText->SetText(mPersonas[nItem]->mAppearance.mUserName);
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

// NTSC-U/C: 0x002c5d10, PAL: 0x002e64a0
void MetMCFreqDelScreen::SetCardSlot(MemcardConnectState slot) {
    mCardSlot = slot;
    mLoadPending = 0;
}
