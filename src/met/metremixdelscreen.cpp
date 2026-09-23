#include "met/metremixdelscreen.h"

#include <vector>

#include "app/playsound.h"
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
#include "met/scrollinglist.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
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
// produces them, and code 7 sets mUnknown100 where code 8 sets mUnknownfc.
constexpr int kCommandCode7 = 7;
constexpr int kCommandCode8 = 8;

// The first catalogue row, and what the back command leaves in MetScreen::mUnknown18.
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
static const char *const kDeleteProgressFormat = "%s %s %s";
static const char *const kCopyDialogue = "remix_copy";
constexpr int kNoButtons = 0;

// The buttons slot 15 acts on, counted from zero in the order the dialogue offers them.
constexpr int kChoiceYes = 1;
constexpr int kChoiceRetry = 0;

// The last argument ListRemixes() takes, which slot 15 leaves clear.
constexpr int kNoPlayList = 0;

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
    return dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszName)));
}

inline Rnd::Font *FindFont(const char *pszName) {
    return dynamic_cast<Rnd::Font *>(Rnd::g_manager.Find(HxStr(pszName)));
}

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A dialogue text read by value from configuration.
inline HxStr ConfigText(const char *pszKey) {
    HxStr value;
    QueryConfigString(&value, kPromptConfigCode, pszKey);
    return value;
}

} // namespace

// 0x003394a0
MetRemixDelScreen::MetRemixDelScreen(MetRenderer *pRenderer, int nPriority)
    : MetSaveRemix(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknownf4(nullptr), mUnknownfc(0), mUnknown100(0), mUnknown104(nullptr),
      mUnknown138(nullptr), mUnknown13c(nullptr) {
    mUnknown60 = 0;
    mUnknown38.push_back(HxStr(kDeleteObjectName));
}

// 0x003397a8
MetRemixDelScreen::~MetRemixDelScreen() {
    delete mUnknownf4;
}

// 0x00343f30
MetRemixDelScreen *MetRemixDelScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRemixDelScreen(pRenderer, nPriority);
}

// 0x0033a098
void MetRemixDelScreen::EnterAndShow() {
    SetShowing(0);
    mUnknowne0 = 0;
    mUnknown104 = nullptr;
    mUnknowndc = 1;
    mUnknownf0 = &MetRemixManager::shared()->mRemixes[mUnknown108.mPortSlot];
    if (mUnknownfc != 0 || mUnknown100 != 0) {
        mUnknownf4->setSelected(kFirstRow);
    } else {
        if (MetRemixManager::shared()->mListStatus[mUnknown108.mPortSlot] ==
            kMemcardStatusUnknown) {
            std::vector<HxStr> buttons;
            buttons.push_back(HxStr(kOkButton));
            const HxStr format(ConfigText(kNoCardText));
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown108.mSlotName)));
            MetMsgScreen::Show(
                HxStr(kNoRemixDialogue), HxStr(kErrorTitle), text, kOneButton, buttons, this);
            return;
        }
        if (mUnknownf0 == nullptr || mUnknownf0->size() == 0) {
            std::vector<HxStr> buttons;
            buttons.push_back(HxStr(kOkButton));
            const HxStr format(ConfigText(kNoRemixOnCardText));
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown108.mSlotName)));
            MetMsgScreen::Show(
                HxStr(kNoRemixDialogue), HxStr(kErrorTitle), text, kOneButton, buttons, this);
            return;
        }
        Rnd::View *pLine = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kLineView)));
        Rnd::Mesh *pHighlight =
            dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kHighlightMesh)));
        Rnd::Mesh *pUpArrow = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kUpArrowMesh)));
        Rnd::Mesh *pDownArrow =
            dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr(kDownArrowMesh)));
        // Yes, the binary does not delete a list an earlier entry left behind.
        mUnknownf4 = new ScrollingList(
            this, kRowPitch, kRowCount, pLine, pHighlight, pUpArrow, pDownArrow, kListContext);
    }
    mUnknownfc = 0;
    mUnknown100 = 0;
    mUnknownf4->setItemCount(mUnknownf0->size());
    mUnknownf4->refresh();
    PushNamedScreen(HxStr(kHelpScreen));
    MetHelpScreen::SelectPreset(HxStr(kOnlyBackPreset));
    MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
    HxStr format;
    QueryConfigString(&format, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(
        HxStr(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown108.mSlotName))));
    PushNamedScreen(HxStr(kDataScreen));
    MetScreen::EnterAndShow();
}

// 0x00339880
void MetRemixDelScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    Rnd::Text *pTitle = FindText(kListTitleObject); // Yes, the binary does not test it for null.
    HxStr title;
    QueryConfigString(&title, kPromptConfigCode, kListTitleKey);
    pTitle->SetText(title);
    pTitle->SetShowing(1);
    mUnknown138 = FindFont(kRowFont);
    mUnknown13c = FindFont(kDimRowFont);
}

// 0x00339b30
void MetRemixDelScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        if (mUnknownf4->getSelected() <= kFirstRow) {
            return;
        }
        mUnknownf4->scrollUp();
        ShowRowOnDataScreen(mUnknownf4->getSelected());
        break;
    case kMetScreenCommandNext:
        if (!(static_cast<unsigned>(mUnknownf4->getSelected()) < mUnknownf0->size() - 1)) {
            return;
        }
        mUnknownf4->scrollDown();
        ShowRowOnDataScreen(mUnknownf4->getSelected());
        break;
    case kMetScreenCommandBack:
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kDataScreen));
        BeginExit();
        break;
    case kCommandCode7:
        if (mUnknownf0->size() == 0) {
            return;
        }
        PlaySoundByName(kToggleSound);
        mUnknown100 = 1;
        MetHelpScreen::SetText(HxStr(kNoText), mUnknown10->mUnknown68);
        ExitScreenByName(HxStr(kDataScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;
    case kCommandCode8:
        if (mUnknownf0->size() == 0) {
            return;
        }
        PlaySoundByName(kToggleSound);
        mUnknownfc = 1;
        MetHelpScreen::SetText(HxStr(kNoText), mUnknown10->mUnknown68);
        ExitScreenByName(HxStr(kDataScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;
    default:
        break;
    }
}

// 0x0033cc78
int MetRemixDelScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    if (static_cast<unsigned>(nItem) < mUnknownf0->size()) {
        MetRemixRecord record((*mUnknownf0)[nItem]);
        pText->SetText(HxStr(record.name));
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

// 0x00343f10
int MetRemixDelScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}

// 0x00344078
void MetRemixDelScreen::OnUnknownSlot7() {
    if (mUnknowne0 != 0) {
        mUnknowne0 = 0;
        OnUnknownSlot40();
    }
}

// 0x003441a0
void MetRemixDelScreen::OnMsgScreenShown(const HxStr &) {
    ActivateNamedPanel(HxStr(kMsgScreenName));
}

// 0x00344058
void MetRemixDelScreen::OnUnknownSlot33() {
    ShowRowOnDataScreen(0);
}

// 0x0033e5c0
void MetRemixDelScreen::OnUnknownSlot40() {
    PushNamedScreen(HxStr(kOwnScreenName));
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

// 0x0033e6d8
void MetRemixDelScreen::OnUnknownSlot41() {
    PushNamedScreen(HxStr(kOwnScreenName));
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

inline void MetRemixDelScreen::StartDelete() {
    std::vector<HxStr> buttons;
    const HxStr first(ConfigText(kDeleteProgressFirst));
    const HxStr second(ConfigText(kDeleteProgressSecond));
    const HxStr text(FormatString(kDeleteProgressFormat,
                                  TextOrEmpty(first),
                                  TextOrEmpty(mUnknown108.mSlotName),
                                  TextOrEmpty(second)));
    MetMsgScreen::Show(
        HxStr(kDeleteDialogue), HxStr(kDeleteTitle), text, kNoButtons, buttons, this);
    const MetRemixRecord record((*mUnknownf0)[mUnknownf4->getSelected()]);
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateDeleteRemixTask(mUnknown108.mPortSlot, record.unknown00_);
}

// 0x0033b280
void MetRemixDelScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kDeleteAskDialogue) {
        if (nChoice == kChoiceYes) {
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
            mUnknown104 = &(*mUnknownf0)[mUnknownf4->getSelected()];
            mUnknown120 = NextCardSlot(mUnknown108);
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateLoadRemixTask(mUnknown108.mPortSlot, mUnknown104->name);
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
        mUnknownf4->setItemCount(mUnknownf0->size());
        mUnknownf4->refresh();
        std::vector<HxStr> screens;
        screens.resize(kReturnScreenCount);
        screens[kReturnScreenSelf] = kOwnScreenName;
        screens[kReturnScreenHelp] = kHelpScreen;
        std::vector<MemcardConnectState> slots;
        slots.push_back(mUnknown108);
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
        mUnknown10->RemoveScreen(this);
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kCardTypeScreen));
        ActivateNamedPanel(HxStr(kCardTypeScreen));
    } else {
        MetSaveRemix::OnMsgScreenDismissed(name, nChoice);
    }
}

// 0x0033ce00
void MetRemixDelScreen::OnUnknownSlot36() {
    if (mUnknownfc == 0 && mUnknown100 == 0) {
        delete mUnknownf4;
        mUnknownf4 = nullptr;
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kCardTypeScreen));
        ActivateNamedPanel(HxStr(kCardTypeScreen));
        return;
    }
    if (mUnknown100 != 0) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        const HxStr format(ConfigText(kCopyAskDialogue));
        const MemcardConnectState target(NextCardSlot(mUnknown108));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(target.mSlotName)));
        MetMsgScreen::Show(
            HxStr(kCopyAskDialogue), HxStr(kCopyTitle), text, kTwoButtons, buttons, this);
        return;
    }
    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kNoButton));
    buttons.push_back(HxStr(kYesButton));
    MetMsgScreen::Show(HxStr(kDeleteAskDialogue),
                       HxStr(kDeleteTitle),
                       ConfigText(kDeleteAskText),
                       kTwoButtons,
                       buttons,
                       this);
}

// 0x0033dec0
void MetRemixDelScreen::OnRemixLoaded([[maybe_unused]] int nPortSlot, int nStatus) {
    if (nStatus != kMemcardStatusOk) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kOkButton));
        MetMsgScreen::Show(HxStr(kCopyFailDialogue),
                           HxStr(kErrorTitle),
                           ConfigText(kCopyFailText),
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
    RecordPendingSave(mUnknown120,
                      kAnyPad,
                      mUnknown104->name,
                      mUnknown104->unknown00_,
                      mUnknown104->appearances,
                      mUnknown104->unknown34_);
}

// 0x0033d768
void MetRemixDelScreen::OnRemixDeleted([[maybe_unused]] int nPortSlot, int nStatus) {
    if (nStatus == kMemcardStatusOk) {
        ExitScreenByName(HxStr(kMsgScreenName));
        return;
    }
    if (nStatus == kMemcardStatusUnknown) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kCancelButton));
        const HxStr format(ConfigText(kDeleteNoCardDialogue));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown108.mSlotName)));
        MetMsgScreen::ShowActive(
            HxStr(kDeleteNoCardDialogue), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
        return;
    }
    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kContinueButton));
    MetMsgScreen::ShowActive(HxStr(kDeleteDialogue),
                             HxStr(kErrorTitle),
                             ConfigText(kDeleteFailText),
                             kOneButton,
                             buttons,
                             this);
}

// 0x0033e7f0
void MetRemixDelScreen::OnUnknownSlot42() {
    mUnknowne0 = 1;
    MetKeyboardRequest request(
        HxStr(kOwnScreenName), HxStr(kKeyboardPrompt), mUnknownb8, kAnyPad, this);
    request.mMaxWidth = kKeyboardMaxWidth;
    request.mMaxLength = kKeyboardMaxLength;
    request.mTicker = kKeyboardTicker;
    MetKeyboardScreen::Open(request);
}

// 0x00343fb8
void MetRemixDelScreen::SetCardSlot(MemcardConnectState slot) {
    mUnknown108 = slot;
}

// 0x003440b8
void MetRemixDelScreen::ShowRowOnDataScreen(int nIndex) {
    // Yes, the binary takes the registered screen without a cast check.
    MetRemixDataScreen *pDataScreen =
        static_cast<MetRemixDataScreen *>(MetScreen::FindScreenByName(HxStr(kDataScreen)));
    // Yes, the binary does not test mUnknownf0 for null here.
    if (!(static_cast<unsigned>(nIndex) < mUnknownf0->size())) {
        pDataScreen->SetRecordShowing(0);
    } else {
        pDataScreen->ShowRecord(&(*mUnknownf0)[nIndex]);
    }
}

// 0x00344240
void MetRemixDelScreen::OnUnknownSlot2(const HxStr &text) {
    if (mUnknowne0 != 0) {
        MetSaveRemix::OnUnknownSlot2(text);
        mUnknowne0 = 0;
    }
}
