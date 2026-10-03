#include "met/metremixdelscreen.h"

#include <vector>

#include "app/playsound.h"
#include "game/globalsettings.h"
#include "memcard/memcardmanager.h"
#include "memcard/memcardop.h"
#include "met/methelpscreen.h"
#include "met/metkeyboardrequest.h"
#include "met/metkeyboardscreen.h"
#include "met/metmsgscreen.h"
#include "met/metremixdatascreen.h"
#include "met/metremixmanager.h"
#include "met/metremixrecord.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "met/scrollinglist.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "rnd/font.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "rnd/view.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcrd";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_remix_del";

// The one container object the screen registers.
static const char *const kDeleteObjectName = "mem_del_remix";

// This screen's own registry key, and the key of the panel slot 16 activates.
static const char *const kOwnScreenName = "MetRemixDelScreen";
static const char *const kMsgScreenName = "MetMsgScreen";

// The text a row past the end of the catalogue shows, and the text slot 19 clears the help panel
// to.
static const char *const kNoText = "";

// Screens slot 19 exits, and the sound codes 7 and 8 play.
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kDataScreen = "MetRemixDataScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kToggleSound = "SND_MET_FM_TOGGLE";

// The two command codes above six this screen acts on. The input translator at `0x002e3738`
// produces them, and code 7 sets mCopyPending where code 8 sets mDeletePending.
constexpr int kCommandCopy = 7;
constexpr int kCommandDelete = 8;

// The first catalogue row, and what the back command writes to MetScreen::mExitChoice.
constexpr int kFirstRow = 0;
constexpr int kExitBack = 0;

// The objects slot 38 resolves, and the configuration string the title shows.
static const char *const kListTitleObject = "mcrd_listpan_title.txt";
static const char *const kListTitleKey = "mcrf_remix";
static const char *const kRowFont = "font1_pink_2";
static const char *const kDimRowFont = "font1_pinkgrey_2";
constexpr int kPromptConfigCode = 600;

// The dialogues MemcardUser slot 16 raises, their title and buttons, and the button counts.
static const char *const kDeleteNoCardDialogue = "del_fail_nocard";
static const char *const kDeleteDialogue = "del_remix";
static const char *const kDeleteFailText = "del_fail";
static const char *const kErrorTitle = "ERROR";
static const char *const kRetryButton = "RETRY";
static const char *const kCancelButton = "CANCEL";
static const char *const kContinueButton = "CONTINUE";

// The dialogue slot 5 raises when the card has no remix list, and its two texts.
static const char *const kNoRemixDialogue = "no_remix";
static const char *const kNoCardText = "mc_load_fail_no_card";
static const char *const kNoRemixOnCardText = "no_remix_on_card";

// The objects slot 5 hands to the ScrollingList, and the list's geometry.
static const char *const kLineView = "mcrd_line.view";
static const char *const kHighlightMesh = "mcrd_hilite.mesh";
static const char *const kUpArrowMesh = "mcrd_up.mesh";
static const char *const kDownArrowMesh = "mcrd_down.mesh";
constexpr int kRowPitch = 22;
constexpr int kRowCount = 15;
constexpr int kListContext = 0;

// The help preset and the title slot 5 shows.
static const char *const kOnlyBackPreset = "only_back_title";
static const char *const kTitleKey = "mem_del_type";
constexpr int kTitleConfigCode = 617;

// What slot 36 pushes when neither confirmation is pending, and the two confirmations it raises.
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kCardTypeScreen = "MetMemCardTypeScreen";
static const char *const kCopyAskDialogue = "remix_copy_ask";
static const char *const kCopyTitle = "COPY";
static const char *const kDeleteAskDialogue = "del_remix_ask";
static const char *const kDeleteTitle = "DELETE";
static const char *const kDeleteAskText = "remix_del_ask";
static const char *const kNoButton = "NO";
static const char *const kYesButton = "YES";

// The progress dialogue slot 15 raises while a delete runs, its two text halves around the slot
// name, and the dialogues slot 15 answers besides its own confirmations.
static const char *const kDeleteProgressFirst = "remix_del1";
static const char *const kDeleteProgressSecond = "remix_del2";
#ifdef VIDEO_STANDARD_PAL
static const char *const kDeleteProgressFormat = "%s%s%s";
#else
static const char *const kDeleteProgressFormat = "%s %s %s";
#endif
static const char *const kCopyDialogue = "remix_copy";
constexpr int kNoButtons = 0;

// The buttons slot 15 acts on, counted from zero in the order the dialogue offers them.
constexpr int kChoiceYes = 1;
constexpr int kChoiceRetry = 0;

// The last argument ListRemixes() takes, which slot 15 leaves clear.
constexpr int kNoPlayList = 0;

#ifdef VIDEO_STANDARD_PAL
// The card location whose state the European release refreshes after a delete.
constexpr int kFirstPortSlot = 0;
#endif

// The dialogue MemcardUser slot 12 raises on a failed load, and its one button.
static const char *const kCopyFailDialogue = "remix_copy_fail";
static const char *const kCopyFailText = "copy_fail_general";
static const char *const kOkButton = "OK";

// The two screens slot 12 names in a list it never reads.
constexpr int kReturnScreenCount = 2;
constexpr int kReturnScreenSelf = 0;
constexpr int kReturnScreenHelp = 1;
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;

// The keyboard request slot 42 builds for a remix name.
static const char *const kKeyboardPrompt = "Remix name";
static const char *const kKeyboardTicker = "met_save_remix_screen_ticker_tape";
constexpr int kKeyboardMaxWidth = 228;
constexpr int kKeyboardMaxLength = 32;
constexpr int kAnyPad = -1;

inline Rnd::Text *FindText(const char *pszName) {
    return dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(pszName)));
}

inline Rnd::Font *FindFont(const char *pszName) {
    return dynamic_cast<Rnd::Font *>(Rnd::TheManager.Find(HxStr(pszName)));
}

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A dialogue text read by value from configuration.
inline HxStr ConfigText(MetStringId nId, const char *pszKey) {
    HxStr value = MetConfigText(nId, kPromptConfigCode, pszKey);
    return value;
}

} // namespace

// NTSC-U/C: 0x003394a0, PAL: 0x00363490
MetRemixDelScreen::MetRemixDelScreen(MetRenderer *pRenderer, int nPriority)
    : MetSaveRemix(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mList(nullptr), mDeletePending(0), mCopyPending(0),
#ifdef VIDEO_STANDARD_PAL
      mRefreshCardPending(0),
#endif
      mCopyRecord(nullptr), mRowFont(nullptr), mDimRowFont(nullptr) {
    mShowsLoadedDrawables = 0;
    mHelpKeys.push_back(MetText(kMetStrHMemDelRemix, kDeleteObjectName));
}

// NTSC-U/C: 0x003397a8, PAL: 0x00363830
MetRemixDelScreen::~MetRemixDelScreen() {
    delete mList;
}

// NTSC-U/C: 0x00343f30, PAL: 0x0036f320
MetRemixDelScreen *MetRemixDelScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRemixDelScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x0033a098, PAL: 0x003642d0
void MetRemixDelScreen::EnterAndShow() {
    SetShowing(0);
    mKeyboardPending = 0;
    mCopyRecord = nullptr;
    mCopying = 1;
    mCatalogue = &MetRemixManager::shared()->mRemixes[mCardSlot.mPortSlot];
    if (mDeletePending != 0 || mCopyPending != 0) {
        mList->setSelected(kFirstRow);
    } else {
        if (MetRemixManager::shared()->mListStatus[mCardSlot.mPortSlot] == kMemcardStatusUnknown) {
            std::vector<HxStr> buttons;
            buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
            const HxStr format(ConfigText(kMetStrMcLoadFailNoCard, kNoCardText));
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
            MetMsgScreen::Show(HxStr(kNoRemixDialogue),
                               MetText(kMetStrMsgERROR, kErrorTitle),
                               text,
                               kOneButton,
                               buttons,
                               this);
            return;
        }
        if (mCatalogue == nullptr || mCatalogue->size() == 0) {
            std::vector<HxStr> buttons;
            buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
            const HxStr format(ConfigText(kMetStrNoRemixOnCard, kNoRemixOnCardText));
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
            MetMsgScreen::Show(HxStr(kNoRemixDialogue),
                               MetText(kMetStrMsgERROR, kErrorTitle),
                               text,
                               kOneButton,
                               buttons,
                               this);
            return;
        }
        Rnd::View *pLine = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kLineView)));
        Rnd::Mesh *pHighlight =
            dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(HxStr(kHighlightMesh)));
        Rnd::Mesh *pUpArrow = dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(HxStr(kUpArrowMesh)));
        Rnd::Mesh *pDownArrow =
            dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(HxStr(kDownArrowMesh)));
        // Yes, the binary does not delete a list an earlier entry left behind.
        mList = new ScrollingList(
            this, kRowPitch, kRowCount, pLine, pHighlight, pUpArrow, pDownArrow, kListContext);
    }
    mDeletePending = 0;
    mCopyPending = 0;
    mList->setItemCount(mCatalogue->size());
    mList->refresh();
    PushNamedScreen(HxStr(kHelpScreen));
    MetHelpScreen::SelectPreset(MetText(kMetStrHOnlyBackTitle, kOnlyBackPreset));
    MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
    HxStr format = MetConfigText(kMetStrTMemDelType, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(
        HxStr(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName))));
    PushNamedScreen(HxStr(kDataScreen));
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x00339880, PAL: 0x00363928
void MetRemixDelScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    Rnd::Text *pTitle = FindText(kListTitleObject); // Yes, the binary does not test it for null.
    HxStr title = MetConfigText(kMetStrMcrfRemix, kPromptConfigCode, kListTitleKey);
    pTitle->SetText(title);
    pTitle->SetShowing(1);
    mRowFont = FindFont(kRowFont);
    mDimRowFont = FindFont(kDimRowFont);
}

// NTSC-U/C: 0x00339b30, PAL: 0x00363c50
void MetRemixDelScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        if (mList->getSelected() <= kFirstRow) {
            return;
        }
        mList->scrollUp();
        ShowRowOnDataScreen(mList->getSelected());
        break;
    case kMetScreenCommandNext:
        if (!(static_cast<unsigned>(mList->getSelected()) < mCatalogue->size() - 1)) {
            return;
        }
        mList->scrollDown();
        ShowRowOnDataScreen(mList->getSelected());
        break;
    case kMetScreenCommandBack:
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kDataScreen));
        BeginExit();
        break;
    case kCommandCopy:
        if (mCatalogue->size() == 0) {
            return;
        }
        PlaySoundByName(kToggleSound);
        mCopyPending = 1;
        MetHelpScreen::SetText(HxStr(kNoText), mRenderer->mAnimationFrame);
        ExitScreenByName(HxStr(kDataScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;
    case kCommandDelete:
        if (mCatalogue->size() == 0) {
            return;
        }
        PlaySoundByName(kToggleSound);
        mDeletePending = 1;
        MetHelpScreen::SetText(HxStr(kNoText), mRenderer->mAnimationFrame);
        ExitScreenByName(HxStr(kDataScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;
    default:
        break;
    }
}

// NTSC-U/C: 0x0033cc78, PAL: 0x003676d0
int MetRemixDelScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    if (static_cast<unsigned>(nItem) < mCatalogue->size()) {
        MetRemixRecord record((*mCatalogue)[nItem]);
        pText->SetText(HxStr(record.name));
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

// NTSC-U/C: 0x00343f10, PAL: 0x0036f300
int MetRemixDelScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}

// NTSC-U/C: 0x00344078, PAL: 0x0036f488
void MetRemixDelScreen::OnPanelActivated() {
#ifdef VIDEO_STANDARD_PAL
    mRefreshCardPending = 0;
#endif
    if (mKeyboardPending != 0) {
        mKeyboardPending = 0;
        OnSaveAbandoned();
    }
}

// NTSC-U/C: 0x003441a0, PAL: 0x0036f4c8
void MetRemixDelScreen::OnMsgScreenShown(const HxStr &) {
    ActivateNamedPanel(HxStr(kMsgScreenName));
}

// NTSC-U/C: 0x00344058, PAL: 0x0036f468
void MetRemixDelScreen::OnEnterFinished() {
    ShowRowOnDataScreen(0);
}

// NTSC-U/C: 0x0033e5c0, PAL: 0x00369678
void MetRemixDelScreen::OnSaveAbandoned() {
    PushNamedScreen(HxStr(kOwnScreenName));
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

// NTSC-U/C: 0x0033e6d8, PAL: 0x003697d8
void MetRemixDelScreen::OnSaveDialogueClosed() {
    PushNamedScreen(HxStr(kOwnScreenName));
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

inline void MetRemixDelScreen::StartDelete() {
    std::vector<HxStr> buttons;
    const HxStr first(ConfigText(kMetStrRemixDel1, kDeleteProgressFirst));
    const HxStr second(ConfigText(kMetStrRemixDel2, kDeleteProgressSecond));
    const HxStr text(FormatString(kDeleteProgressFormat,
                                  TextOrEmpty(first),
                                  TextOrEmpty(mCardSlot.mSlotName),
                                  TextOrEmpty(second)));
    MetMsgScreen::Show(HxStr(kDeleteDialogue),
                       MetText(kMetStrMsgDELETE, kDeleteTitle),
                       text,
                       kNoButtons,
                       buttons,
                       this);
    const MetRemixRecord record((*mCatalogue)[mList->getSelected()]);
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateDeleteRemixTask(mCardSlot.mPortSlot, record.name);
}

// NTSC-U/C: 0x0033b280, PAL: 0x00365798
void MetRemixDelScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kDeleteAskDialogue) {
        if (nChoice == kChoiceYes) {
#ifdef VIDEO_STANDARD_PAL
            // The European release logs only this path, while it builds the progress text.
            LogPrintf("ok ay to delete remix\n");
#endif
            StartDelete();
        } else {
            PushNamedScreen(HxStr(kHelpScreen));
            PushNamedScreen(HxStr(kTitleScreen));
            PushNamedScreen(HxStr(kDataScreen));
            PushNamedScreen(HxStr(kOwnScreenName));
            ActivateNamedPanel(HxStr(kOwnScreenName));
        }
    } else if (name == kCopyAskDialogue) {
        if (nChoice == kChoiceYes) {
            mCopyRecord = &(*mCatalogue)[mList->getSelected()];
            mCopyTarget = NextCardSlot(mCardSlot);
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateLoadRemixTask(mCardSlot.mPortSlot, mCopyRecord->name);
        } else {
            PushNamedScreen(HxStr(kHelpScreen));
            PushNamedScreen(HxStr(kTitleScreen));
            PushNamedScreen(HxStr(kDataScreen));
            PushNamedScreen(HxStr(kOwnScreenName));
            ActivateNamedPanel(HxStr(kOwnScreenName));
        }
    } else if (name == kDeleteNoCardDialogue) {
        if (nChoice == kChoiceRetry) {
            StartDelete();
        } else {
            PushNamedScreen(HxStr(kTitleScreen));
            PushNamedScreen(HxStr(kHelpScreen));
            PushNamedScreen(HxStr(kDataScreen));
            PushNamedScreen(HxStr(kOwnScreenName));
            ActivateNamedPanel(HxStr(kOwnScreenName));
        }
    } else if (name == kDeleteDialogue) {
        mList->setItemCount(mCatalogue->size());
        mList->refresh();
        std::vector<HxStr> screens;
        screens.resize(kReturnScreenCount);
        screens[kReturnScreenSelf] = kOwnScreenName;
        screens[kReturnScreenHelp] = kHelpScreen;
        std::vector<MemcardConnectState> slots;
        slots.push_back(mCardSlot);
        MetRemixManager::shared()->ListRemixes(screens, slots, kNoPlayList);
    } else if (name == kCopyDialogue) {
        PushNamedScreen(HxStr(kTitleScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kDataScreen));
        PushNamedScreen(HxStr(kOwnScreenName));
        ActivateNamedPanel(HxStr(kOwnScreenName));
    } else if (name == kCopyFailDialogue) {
        PushNamedScreen(HxStr(kTitleScreen));
        PushNamedScreen(HxStr(kDataScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kOwnScreenName));
        ActivateNamedPanel(HxStr(kOwnScreenName));
    } else if (name == kNoRemixDialogue) {
        mRenderer->RemoveScreen(this);
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kCardTypeScreen));
        ActivateNamedPanel(HxStr(kCardTypeScreen));
    } else {
        MetSaveRemix::OnMsgScreenDismissed(name, nChoice);
    }
}

// NTSC-U/C: 0x0033ce00, PAL: 0x00367898
void MetRemixDelScreen::OnExitFinished() {
    if (mDeletePending == 0 && mCopyPending == 0) {
        delete mList;
        mList = nullptr;
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kCardTypeScreen));
        ActivateNamedPanel(HxStr(kCardTypeScreen));
        return;
    }
    if (mCopyPending != 0) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
        buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
        const HxStr format(ConfigText(kMetStrRemixCopyAsk, kCopyAskDialogue));
        const MemcardConnectState target(NextCardSlot(mCardSlot));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(target.mSlotName)));
        MetMsgScreen::Show(HxStr(kCopyAskDialogue),
                           MetText(kMetStrMsgCOPY, kCopyTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        return;
    }
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
    buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
    MetMsgScreen::Show(HxStr(kDeleteAskDialogue),
                       MetText(kMetStrMsgDELETE, kDeleteTitle),
                       ConfigText(kMetStrRemixDelAsk, kDeleteAskText),
                       kTwoButtons,
                       buttons,
                       this);
}

// NTSC-U/C: 0x0033dec0, PAL: 0x00368ea0
void MetRemixDelScreen::OnRemixLoaded([[maybe_unused]] int nPortSlot, int nStatus) {
    if (nStatus != kMemcardStatusOk) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
        MetMsgScreen::Show(HxStr(kCopyFailDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           ConfigText(kMetStrCopyFailGeneral, kCopyFailText),
                           kOneButton,
                           buttons,
                           this);
        return;
    }
    // Yes, the binary builds this list and destroys it without reading it.
    std::vector<HxStr> screens;
    screens.resize(kReturnScreenCount);
    screens[kReturnScreenSelf] = kOwnScreenName;
    screens[kReturnScreenHelp] = kHelpScreen;
    MemcardManager::shared()->mUser = this;
    RecordPendingSave(mCopyTarget,
                      kAnyPad,
                      mCopyRecord->name,
                      mCopyRecord->levelName,
                      mCopyRecord->appearances,
                      mCopyRecord->albumNumber);
}

// NTSC-U/C: 0x0033d768, PAL: 0x003685d8
void MetRemixDelScreen::OnRemixDeleted([[maybe_unused]] int nPortSlot, int nStatus) {
    if (nStatus == kMemcardStatusOk) {
#ifdef VIDEO_STANDARD_PAL
        if (nPortSlot == kFirstPortSlot) {
            GlobalSettings::shared(); // Yes, the binary discards this call's result.
            mRefreshCardPending = 1;
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateGetConnectStateTask(kFirstPortSlot);
            return;
        }
#endif
        ExitScreenByName(HxStr(kMsgScreenName));
        return;
    }
    if (nStatus == kMemcardStatusUnknown) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCANCEL, kCancelButton));
        const HxStr format(ConfigText(kMetStrDelFailNocard, kDeleteNoCardDialogue));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mCardSlot.mSlotName)));
        MetMsgScreen::ShowActive(HxStr(kDeleteNoCardDialogue),
                                 MetText(kMetStrMsgERROR, kErrorTitle),
                                 text,
                                 kTwoButtons,
                                 buttons,
                                 this);
        return;
    }
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
    MetMsgScreen::ShowActive(HxStr(kDeleteDialogue),
                             MetText(kMetStrMsgERROR, kErrorTitle),
                             ConfigText(kMetStrDelFail, kDeleteFailText),
                             kOneButton,
                             buttons,
                             this);
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x00368378
void MetRemixDelScreen::OnConnectState(MemcardConnectState state, int nStatus) {
    if (nStatus != kMemcardStatusOk || state.mFormatted == 0 || mRefreshCardPending == 0) {
        MetSaveRemix::OnConnectState(state, nStatus);
        return;
    }
    GlobalSettings::shared()->mCardSlots[0] = state;
    mRefreshCardPending = 0;
    ExitScreenByName(HxStr(kMsgScreenName));
}
#endif

// NTSC-U/C: 0x0033e7f0, PAL: 0x00369938
void MetRemixDelScreen::OnDuplicateNameDeclined() {
    mKeyboardPending = 1;
    MetKeyboardRequest request(HxStr(kOwnScreenName),
                               MetText(kMetStrHKbRemixSave, kKeyboardPrompt),
                               mRemixName,
                               kAnyPad,
                               this);
    request.mMaxWidth = kKeyboardMaxWidth;
    request.mMaxLength = kKeyboardMaxLength;
    request.mTicker = MetText(kMetStrHMetSaveRemixScreenTickerTape, kKeyboardTicker);
    MetKeyboardScreen::Open(request);
}

// NTSC-U/C: 0x00343fb8, PAL: 0x0036f3a8
void MetRemixDelScreen::SetCardSlot(MemcardConnectState slot) {
    mCardSlot = slot;
}

// NTSC-U/C: 0x003440b8, PAL: 0x00365690
void MetRemixDelScreen::ShowRowOnDataScreen(int nIndex) {
    // Yes, the binary takes the registered screen without a cast check.
    MetRemixDataScreen *pDataScreen =
        static_cast<MetRemixDataScreen *>(MetScreen::FindScreenByName(HxStr(kDataScreen)));
    // Yes, the binary does not test mCatalogue for null here.
    if (!(static_cast<unsigned>(nIndex) < mCatalogue->size())) {
        pDataScreen->SetRecordShowing(0);
    } else {
        pDataScreen->ShowRecord(&(*mCatalogue)[nIndex]);
    }
}

// NTSC-U/C: 0x00344240, PAL: 0x0036f588
void MetRemixDelScreen::OnKeyboardTextEntered(const HxStr &text) {
    if (mKeyboardPending != 0) {
        MetSaveRemix::OnKeyboardTextEntered(text);
        mKeyboardPending = 0;
    }
}
