#include "met/metremixmanager.h"

#include <algorithm>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/globalsettings.h"
#include "memcard/memcardconnectstate.h"
#include "memcard/memcardmanager.h"
#include "memcard/remixindex.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "msg/metfreqendedmsg.h"
#include "os/async.h"
#include "os/formatstring.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/r250.h"
#include "os/zone.h"
#include "script/configquery.h"
#include "script/scripthost.h"
#include "stream/iobmemstream.h"
#include "stream/iobpreallocmemstream.h"
#include "synth/ps2hardsynth.h"

namespace {

// The screen name, directory, and container the manager registers under.
constexpr char kScreenName[] = "dlg";
constexpr char kDirectory[] = "metagame/Shared";
constexpr char kContainerName[] = "dialogue";

// The registry key shared() resolves the instance by.
constexpr char kRegistryName[] = "MetRemixManager";

// The message screen the listing and playlist completions exit.
constexpr char kMsgScreen[] = "MetMsgScreen";

// The two listing statuses OnRemixesListed() tests. The meaning of 3 is inferred.
constexpr int kListStatusOk = 0;
constexpr int kListStatusNoRemixes = 3;

// The status ListRemixes() records for a card slot until OnRemixesListed() replaces it. It is the
// same value as kCardStatusNoCard.
constexpr int kListStatusPending = 15;

// ListRemixes() builds its warning from the factory text and the card text when both apply.
constexpr int kBothSources = 2;

// What Done() does once a remix read completes, as mAfterLoadAction records it.
constexpr int kAfterLoadStart = 0;
constexpr int kAfterLoadExit = 1;

// The origin Done() rewinds the reset log to.
constexpr int kSeekFromStart = 0;

// The configuration code the dialogue texts are read under, and the code of the playlist number
// the save and load tasks include.
constexpr int kDialogueConfigCode = 600;
constexpr int kPlayListIndexCode = 0x514;

// Dialogue names and the title they share.
constexpr char kMemSaveDialogue[] = "mem_save";
constexpr char kWarningTitle[] = "WARNING";
constexpr char kRemixLoadFailedDialogue[] = "remix_load_failed";
constexpr char kLoadFailText[] = "load_fail";
constexpr char kFormatCheckDialogue[] = "mem_format_check";
constexpr char kPlayListSaveFailedDialogue[] = "playlist_save_failed";
constexpr char kPlayListSaveRetryDialogue[] = "playlist_save_failed_tryagain";
constexpr char kSaveFailText[] = "save_fail";
constexpr char kSaveFailNoCardText[] = "save_fail_nocard";
constexpr char kSaveFailNoSpaceText[] = "save_fail_nospace";
constexpr char kLoadRemixDataDialogue[] = "mem_load_remix_data";
constexpr char kMemLoadDialogue[] = "mem_load";
constexpr char kFormatGoDialogue[] = "mem_format_go";
constexpr char kFormatDoneDialogue[] = "mem_format_done";
constexpr char kFormatFailDialogue[] = "format_fail";
constexpr char kFactoryListLoadText[] = "fact_list_load";
constexpr char kSlotNameSeparator[] = ", ";
constexpr char kBothSourcesFormat[] = "%s %s";
constexpr char kFactoryCustomLoadText[] = "fact_custom_load";
constexpr char kFactoryRemixLoadText[] = "fact_remix_load";
constexpr char kCustomLoadText[] = "custom_load";
constexpr char kRemixLoadText[] = "remix_load";
constexpr char kFormatSuccessText[] = "format_success";
constexpr char kFormatAlreadyText[] = "format_already";
constexpr char kFormatTitle[] = "FORMAT";
constexpr char kSettingsTitle[] = "SETTINGS";
constexpr char kErrorTitle[] = "ERROR";
#ifdef VIDEO_STANDARD_PAL
constexpr char kSaveAbortedDialogue[] = "save_fail_no_format";

// The packed port and slot of port 1, and the slot name the European release gives it when
// GlobalSettings records another card.
constexpr int kPortOne = 0;
constexpr char kPortOneSlotName[] = "1";
#endif

// Button labels, and the button counts MetMsgScreen::Show() receives with them.
constexpr char kOkButton[] = "OK";
constexpr char kNoButton[] = "NO";
constexpr char kYesButton[] = "YES";
constexpr char kRetryButton[] = "RETRY";
constexpr char kContinueButton[] = "CONTINUE";
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;

// The dialogue buttons OnMsgScreenDismissed() tests, counted from zero.
constexpr int kChoiceFirst = 0;
constexpr int kChoiceSecond = 1;

// Memory-card statuses the completion slots branch on. The names are inferred from the dialogue
// each one raises.
constexpr int kCardStatusUnformatted = 1;
constexpr int kCardStatusNoSpace = 2;
constexpr int kCardStatusAlreadyFormatted = 13;
constexpr int kCardStatusNoCard = 15;

// The screens StartPlayList() records for the return from a jukebox game.
constexpr char kTopButtonsScreen[] = "MetJukeboxTopButtonsScreen";
constexpr char kHelpScreen[] = "MetHelpScreen";

// The screen StartLoadedRemix() pushes and activates.
constexpr char kLoadGameScreen[] = "MetLoadGameScreen";

// The script template StartLoadedRemix() runs, and the one argument it passes.
constexpr int kJukeboxStartTemplate = 0x267;
constexpr char kJukeboxStartArgument[] = "1";
constexpr char kJukeboxStopArgument[] = "0";

// 0x006c1100
HxStr g_remixIndexPath("Levels/remixes/ps2/index");

// 0x006c1108
HxStr g_remixDirectory("Levels/remixes/ps2/");

inline const char *PathOrEmpty(const HxStr &path) {
    return path.mStr != nullptr ? path.mStr : g_szEmptyString;
}

#ifdef VIDEO_STANDARD_PAL
// The card GlobalSettings records when it is in port 1, or otherwise port 1 under the slot name
// `1`. The European release expands it in place.
inline MemcardConnectState PortOneCardSlot() {
    MemcardConnectState slot;
    if (GlobalSettings::shared()->mCardSlots.size() != 0 &&
        GlobalSettings::shared()->mCardSlots[0].mPortSlot == kPortOne) {
        slot = GlobalSettings::shared()->mCardSlots[0];
    } else {
        slot.mPortSlot = kPortOne;
        slot.mSlotName = kPortOneSlotName;
    }
    return slot;
}
#endif

// The display name of the first memory-card slot, or the empty string.
inline const char *FirstCardSlotText() {
    return PathOrEmpty(GlobalSettings::shared()->mCardSlots[0].mSlotName);
}

// Replaces a list of screen names by clearing, resizing, and then assigning, the sequence every
// writer of mReturnScreens and mRestoreScreens expands.
inline void ReplaceScreens(std::vector<HxStr> &screens, const std::vector<HxStr> &source) {
    screens.clear();
    screens.resize(source.size());
    screens = source;
}

// 0x00361cb8
inline int Clamp(int nLow, int nValue, int nHigh) {
    if (nValue < nLow) {
        return nLow;
    }
    return nHigh < nValue ? nHigh : nValue;
}

} // namespace

// 0x006c1110
MetRemixManager *MetRemixManager::sInstance;

// 0x00352b80
MetRemixManager::MetRemixManager(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mListingsDone(0), mListingsExpected(0), mPlayListReady(1), mIndexRequest(0), mRemixRequest(0),
      mIndexSlot(-1), mCurrentPlaylistTrack(0), mShuffle(0), mAfterLoadAction(0) {
}

// 0x00355070
MetRemixManager::~MetRemixManager() {
}

// 0x00361020
MetRemixManager *MetRemixManager::New(MetRenderer *pRenderer, int nPriority) {
    return new MetRemixManager(pRenderer, nPriority);
}

// 0x00361000
MetRemixManager *MetRemixManager::shared() {
    return ResolveSharedInstance();
}

// 0x003610a8
MetRemixManager *MetRemixManager::ResolveSharedInstance() {
    CacheSharedInstance();
    return sInstance;
}

// 0x00361210
void MetRemixManager::CacheSharedInstance() {
    if (sInstance == nullptr) {
        sInstance =
            static_cast<MetRemixManager *>(MetScreen::FindScreenByName(HxStr(kRegistryName)));
    }
}

// 0x00361558
MetRemixRecord *MetRemixManager::GetRecord() {
    return &mRecord;
}

// 0x00361560
void MetRemixManager::SetRecord(const MetRemixRecord &record) {
    mRecord = record;
}

// 0x00359230
MetRemixRecord *MetRemixManager::FindRecord(const HxStr &name) {
    for (auto it = mRemixes.begin(); it != mRemixes.end(); ++it) {
        std::vector<MetRemixRecord> &records = it->second;
        const int nRecords = records.size();
        for (int i = 0; i < nRecords; ++i) {
            if (!(records[i].name == name)) {
                continue;
            }
            if (records[i].factory != 0) {
                if (it->first == kFactorySlot) {
                    return &records[i];
                }
            } else if (it->first != kFactorySlot) {
                return &records[i];
            }
        }
    }
    return nullptr;
}

// 0x00357f80
void MetRemixManager::LoadIndex() {
    const HxStr path = GetFreqRoot() + g_remixIndexPath;
    const int nZone = ZoneGetCurrent();
    ZoneSetCurrent(kNoZone);
    mIndexRequest = AsyncLoadFileByPath(PathOrEmpty(path), nullptr, 0, this);
    ZoneSetCurrent(nZone);
}

// 0x003580f0
void MetRemixManager::LoadRemixFile(const HxStr &fileName) {
    const HxStr directory = GetFreqRoot() + g_remixDirectory;
    const HxStr path = directory + fileName;
    const int nZone = ZoneGetCurrent();
    ZoneSetCurrent(kNoZone);
    mRemixRequest = AsyncLoadFileByPath(PathOrEmpty(path), nullptr, 0, this);
    ZoneSetCurrent(nZone);
}

// 0x003613d8
void MetRemixManager::SetCurrentTrack(int nTrack) {
    mCurrentPlaylistTrack = Clamp(0, nTrack, mPlayList.entries.size());
}

// 0x0035a7e0
void MetRemixManager::RandomTrack() {
    const int nTracks = mPlayList.entries.size();
    int nUnplayed = 0;
    for (int i = 0; i < nTracks; ++i) {
        if (!mPlayedTracks[i]) {
            ++nUnplayed;
        }
    }
    if (nUnplayed == 0) {
        for (int i = 0; i < nTracks; ++i) {
            mPlayedTracks[i] = false;
        }
        nUnplayed = nTracks;
    }

    const int nPick = RandomInt(0, nUnplayed);
    int nTrack = -1;
    int nSeen = 0;
    for (int i = 0; i < nTracks; ++i) {
        if (mPlayedTracks[i]) {
            continue;
        }
        if (nSeen == nPick) {
            nTrack = i;
            break;
        }
        ++nSeen;
    }
    mPlayedTracks[nTrack] = true; // Yes, the binary marks track -1 when none was picked.
    SetCurrentTrack(nTrack);
    LogPrintf(
        "MetRemixManager::%s() - mCurrentPlaylistTrack = %i.\n", __func__, mCurrentPlaylistTrack);
}

// 0x00361300
void MetRemixManager::PreviousTrack() {
    if (mShuffle != 0) {
        RandomTrack();
        return;
    }
    mCurrentPlaylistTrack = std::max(0, mCurrentPlaylistTrack - 1);
}

// 0x00361358
inline void MetRemixManager::NextTrack() {
    if (mShuffle != 0) {
        RandomTrack();
        return;
    }
    const int nLast = mPlayList.entries.size() - 1;
    if (mCurrentPlaylistTrack == nLast) {
        SetCurrentTrack(0);
        return;
    }
    mCurrentPlaylistTrack = std::min(nLast, mCurrentPlaylistTrack + 1);
}

// NTSC-U/C: 0x00353350, PAL: 0x0037f5a8
void MetRemixManager::ListRemixes(const std::vector<HxStr> &returnScreens,
                                  std::vector<MemcardConnectState> slots,
                                  int bLoadPlayList) {
    CacheSharedInstance(); // Yes, the binary resolves this instance first and ignores it.
    ReplaceScreens(mReturnScreens, returnScreens);
    const std::vector<HxStr> buttons;

    std::vector<int> cardSlots;
    int bHasFactory = 0;
    for (unsigned i = 0; i < slots.size(); ++i) {
        if (slots[i].mPortSlot == kFactorySlot) {
            bHasFactory = 1;
        } else {
            cardSlots.push_back(i);
        }
    }

    HxStr text;
    HxStr factoryText;
    HxStr cardText;
    HxStr slotNames;
    int nParts = 0;
    if (bHasFactory != 0) {
        factoryText = MetConfigText(kMetStrFactListLoad, kDialogueConfigCode, kFactoryListLoadText);
        nParts = 1;
    }
    if (cardSlots.size() != 0) {
        ++nParts;
        slotNames = slots[cardSlots[0]].mSlotName;
        for (unsigned i = 1; i < cardSlots.size(); ++i) {
            slotNames.Insert(slotNames.mLen, HxStr(kSlotNameSeparator));
            slotNames.Insert(slotNames.mLen, slots[cardSlots[i]].mSlotName);
        }
        cardText = FormatString(
            PathOrEmpty(MetConfigText(kMetStrMemLoad, kDialogueConfigCode, kMemLoadDialogue)),
            PathOrEmpty(slotNames));
    }
    if (nParts == kBothSources) {
        text = FormatString(kBothSourcesFormat, PathOrEmpty(factoryText), PathOrEmpty(cardText));
    } else if (bHasFactory != 0) {
        text = factoryText;
    } else {
        text = cardText;
    }
    MetMsgScreen::Show(
        HxStr(kMemLoadDialogue), MetText(kMetStrMsgWARNING, kWarningTitle), text, 0, buttons, this);

    MemcardManager::shared()->mUser = this;
    mRemixes.clear();
    mListStatus.clear();
    mListingsDone = 0;
    mCurrentPlaylistTrack = 0;
    const int nSlots = slots.size();
    mListingsExpected = nSlots;
    for (int i = 0; i < nSlots; ++i) {
        if (slots[i].mPortSlot == kFactorySlot) {
            mIndexSlot = slots[i].mPortSlot;
            mRemixes[slots[i].mPortSlot]; // Yes, the binary only creates the factory entry here.
            LoadIndex();
            mListStatus[slots[i].mPortSlot] = kListStatusOk;
        } else {
            MemcardManager *pManager = MemcardManager::shared();
            pManager->CreateListRemixesTask(slots[i].mPortSlot, &mRemixes[slots[i].mPortSlot]);
            mListStatus[slots[i].mPortSlot] = kListStatusPending;
        }
    }

    // Yes, the binary empties the playlist without releasing its entries.
    mPlayList.entries.clear();
    if (bLoadPlayList != 0) {
        mPlayListReady = 0;
        MemcardManager::shared()->CreateLoadJukeboxPlayListTask(
            0, &mPlayList, QueryConfigValue(kPlayListIndexCode));
    }
}

// NTSC-U/C: 0x003548e8, PAL: 0x00380cd8
void MetRemixManager::BeginRemixLoad(const std::vector<HxStr> &returnScreens,
                                     const std::vector<HxStr> &restoreScreens,
                                     const MetRemixRecord &record,
                                     int nFactory) {
    const std::vector<HxStr> buttons;
    HxStr text;
    if (nFactory != 0) {
        if (Application::shared()->GetPlayMode() == kPlayModeGame) {
            text =
                MetConfigText(kMetStrFactCustomLoad, kDialogueConfigCode, kFactoryCustomLoadText);
        } else {
            text = MetConfigText(kMetStrFactRemixLoad, kDialogueConfigCode, kFactoryRemixLoadText);
        }
    } else {
        const HxStr slotName = FirstCardSlotName();
        HxStr format;
        if (Application::shared()->GetPlayMode() == kPlayModeGame) {
            format = MetConfigText(kMetStrCustomLoad, kDialogueConfigCode, kCustomLoadText);
        } else {
            format = MetConfigText(kMetStrRemixLoad, kDialogueConfigCode, kRemixLoadText);
        }
        text = FormatString(PathOrEmpty(format), PathOrEmpty(slotName));
    }
    MetMsgScreen::Show(HxStr(kLoadRemixDataDialogue),
                       MetText(kMetStrMsgWARNING, kWarningTitle),
                       text,
                       0,
                       buttons,
                       this);
    mAfterLoadAction = kAfterLoadExit;
    ReplaceScreens(mReturnScreens, returnScreens);
    ReplaceScreens(mRestoreScreens, restoreScreens);
    LoadRemix(record, nFactory);
}

// NTSC-U/C: 0x00355ff0, PAL: 0x00382920
void MetRemixManager::OnRemixLoaded(int nPortSlot, int nStatus) {
    LogPrintf(" in MetRemixManager::%s(). Return code = %i.\n", __func__, nStatus);
    if (nStatus == 0) {
        if (mAfterLoadAction == kAfterLoadStart) {
            StartLoadedRemix();
        } else if (mAfterLoadAction == kAfterLoadExit) {
            ExitScreenByName(HxStr(kMsgScreen));
        }
        return;
    }

    LogPrintf("Failed to load remix from memory card slot %i.", nPortSlot);
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
#ifdef VIDEO_STANDARD_PAL
    HxStr text;
    if (mAfterLoadAction == kAfterLoadStart) {
        text = GetMetString(kMetStrLoadFailJukebox);
    } else {
        text = GetMetString(kMetStrLoadFail);
    }
    const HxStr name(kRemixLoadFailedDialogue);
    const HxStr title(GetMetString(kMetStrMsgWARNING));
#else
    const HxStr name(kRemixLoadFailedDialogue);
    const HxStr title(kWarningTitle);
    HxStr text = QueryConfigString(kDialogueConfigCode, kLoadFailText);
#endif
    MetMsgScreen::Show(name, title, text, 1, buttons, this);
}

inline void MetRemixManager::RetrySavePlayList() {
    std::vector<HxStr> screens;
    screens = mReturnScreens;
    SavePlayList(screens);
}

// NTSC-U/C: 0x003555c0, PAL: 0x00381b20
void MetRemixManager::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kLoadRemixDataDialogue) {
        PushReturnScreens();
    } else if (name == kMemLoadDialogue) {
        PushReturnScreens();
        mListingsExpected = -1;
        mListingsDone = -1;
    } else if (name == kMemSaveDialogue || name == kPlayListSaveFailedDialogue) {
        PushReturnScreens();
    } else if (name == kPlayListSaveRetryDialogue) {
        if (nChoice != kChoiceFirst) {
            PushReturnScreens();
        } else {
            RetrySavePlayList();
        }
    } else if (name == kRemixLoadFailedDialogue) {
        GameParams params(*Application::shared()->GetGameManager()->GetParams());
        params.mLoadingGame = 0;
        CallScriptTemplate(kJukeboxStartTemplate, kJukeboxStopArgument);
        Application::shared()->GetGameManager()->SetParams(params);
        if (params.mJukeboxMode == 1) {
            Application::shared()->GetSynth()->LoadBankSet4();
            mRenderer->Stop();
            MetFreqEndedMsg msg;
            msg.mStopJukebox = params.mJukeboxMode;
            mRenderer->Handle(&msg);
        } else {
            PushRestoreScreens();
        }
    } else if (name == kFormatCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->CreateFormatTask(0);
            const std::vector<HxStr> buttons;
            GlobalSettings::shared(); // Yes, the binary discards this call's result.
            const HxStr format(
                MetConfigText(kMetStrMemFormatGo, kDialogueConfigCode, kFormatGoDialogue));
            const HxStr text(FormatString(PathOrEmpty(format), FirstCardSlotText()));
            MetMsgScreen::Show(HxStr(kFormatGoDialogue),
                               MetText(kMetStrMsgWARNING, kWarningTitle),
                               text,
                               0,
                               buttons,
                               this);
        } else {
#ifdef VIDEO_STANDARD_PAL
            std::vector<HxStr> buttons;
            buttons.push_back(GetMetString(kMetStrMsgRETRY));
            buttons.push_back(GetMetString(kMetStrMsgCONTINUE));
            MetMsgScreen::Show(HxStr(kSaveAbortedDialogue),
                               GetMetString(kMetStrMsgERROR),
                               GetMetString(kMetStrSaveAborted),
                               kTwoButtons,
                               buttons,
                               this);
#else
            RetrySavePlayList();
#endif
        }
    } else if (name == kFormatDoneDialogue) {
        RetrySavePlayList();
#ifdef VIDEO_STANDARD_PAL
    } else if (name == kFormatFailDialogue || name == kSaveAbortedDialogue) {
#else
    } else if (name == kFormatFailDialogue) {
#endif
        if (nChoice != kChoiceFirst) {
            PushReturnScreens();
        } else {
            RetrySavePlayList();
        }
    }
}

// NTSC-U/C: 0x003569d0, PAL: 0x00383638
void MetRemixManager::OnCardFormatted([[maybe_unused]] int nPortSlot, int nStatus) {
    if (nStatus == 0) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr format(
            MetConfigText(kMetStrFormatSuccess, kDialogueConfigCode, kFormatSuccessText));
        const HxStr text(FormatString(PathOrEmpty(format), FirstCardSlotText()));
        MetMsgScreen::ShowActive(HxStr(kFormatDoneDialogue),
                                 MetText(kMetStrMsgFORMAT, kFormatTitle),
                                 text,
                                 kOneButton,
                                 buttons,
                                 this);
    } else if (nStatus == kCardStatusAlreadyFormatted) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr format(
            MetConfigText(kMetStrFormatAlready, kDialogueConfigCode, kFormatAlreadyText));
        const HxStr text(FormatString(PathOrEmpty(format), FirstCardSlotText()));
        MetMsgScreen::ShowActive(HxStr(kFormatDoneDialogue),
                                 MetText(kMetStrMsgFORMAT, kFormatTitle),
                                 text,
                                 kOneButton,
                                 buttons,
                                 this);
    } else {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        MetMsgScreen::Show(
            HxStr(kFormatFailDialogue),
            MetText(kMetStrMsgERROR, kErrorTitle),
            MetConfigText(kMetStrFormatFail, kDialogueConfigCode, kFormatFailDialogue),
            kTwoButtons,
            buttons,
            this);
    }
}

// NTSC-U/C: 0x003573e8, PAL: 0x003841e0
#ifdef VIDEO_STANDARD_PAL
void MetRemixManager::OnJukeboxPlayListSaved([[maybe_unused]] int nPortSlot,
                                             int nStatus,
                                             int nKilobytes) {
#else
void MetRemixManager::OnJukeboxPlayListSaved([[maybe_unused]] int nPortSlot, int nStatus) {
#endif
    std::vector<HxStr> buttons;
    HxStr format;
    HxStr text;
    switch (nStatus) {
    case 0:
        ExitScreenByName(HxStr(kMsgScreen));
        break;

    case kCardStatusUnformatted:
        buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
        buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
        format = MetConfigText(kMetStrMemFormatCheck, kDialogueConfigCode, kFormatCheckDialogue);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        text = FormatString(PathOrEmpty(format), FirstCardSlotText());
        MetMsgScreen::Show(HxStr(kFormatCheckDialogue),
                           MetText(kMetStrMsgSETTINGS, kSettingsTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        break;

    case kCardStatusNoCard:
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        format = MetConfigText(kMetStrSaveFailNocard, kDialogueConfigCode, kSaveFailNoCardText);
#ifdef VIDEO_STANDARD_PAL
        text = FormatString(PathOrEmpty(format), kPortOneSlotName);
#else
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        text = FormatString(PathOrEmpty(format), FirstCardSlotText());
#endif
        MetMsgScreen::Show(HxStr(kPlayListSaveRetryDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        break;

    case kCardStatusNoSpace:
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        format = MetConfigText(kMetStrSaveFailNospace, kDialogueConfigCode, kSaveFailNoSpaceText);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
#ifdef VIDEO_STANDARD_PAL
        text = FormatString(PathOrEmpty(format), FirstCardSlotText(), nKilobytes);
#else
        text = FormatString(PathOrEmpty(format), FirstCardSlotText());
#endif
        MetMsgScreen::Show(HxStr(kPlayListSaveRetryDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        break;

    default:
        buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
        MetMsgScreen::Show(HxStr(kPlayListSaveFailedDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           MetConfigText(kMetStrSaveFail, kDialogueConfigCode, kSaveFailText),
                           kOneButton,
                           buttons,
                           this);
        break;
    }
}

// NTSC-U/C: 0x00356508, PAL: 0x00382fc8
void MetRemixManager::SavePlayList(const std::vector<HxStr> &returnScreens) {
    ReplaceScreens(mReturnScreens, returnScreens);
    const std::vector<HxStr> buttons;
#ifdef VIDEO_STANDARD_PAL
    HxStr format = GetMetString(kMetStrMemSave);
    const MemcardConnectState slot(PortOneCardSlot());
    const HxStr text(FormatString(PathOrEmpty(format), PathOrEmpty(slot.mSlotName)));
#else
    HxStr format = QueryConfigString(kDialogueConfigCode, kMemSaveDialogue);
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    const HxStr text(FormatString(PathOrEmpty(format), FirstCardSlotText()));
#endif
    MetMsgScreen::Show(
        HxStr(kMemSaveDialogue), MetText(kMetStrMsgWARNING, kWarningTitle), text, 0, buttons, this);
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateSaveJukeboxPlayListTask(
        0, &mPlayList, QueryConfigValue(kPlayListIndexCode));
}

// 0x003593d0
void MetRemixManager::StartPlayList(const std::vector<HxStr> &returnScreens, int nShuffle) {
    mRestoreScreens.clear();
    mRestoreScreens.push_back(HxStr(kTopButtonsScreen));
    mRestoreScreens.push_back(HxStr(kHelpScreen));
    mPlayedTracks.resize(mPlayList.entries.size());
    std::fill(mPlayedTracks.begin(), mPlayedTracks.end(), false);
    SetCurrentTrack(0);
    mShuffle = nShuffle;
    if (nShuffle != 0) {
        RandomTrack();
    }
    ReplaceScreens(mReturnScreens, returnScreens);
    LoadCurrentTrack();
}

// 0x0035abf8
void MetRemixManager::StartLoadedRemix() {
    MetRemixRecord *pRecord = FindRecord(mPlayList.GetEntry(mCurrentPlaylistTrack)->name);
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    const int nArena = RandomInt(0, GetArenaList()->size() - 1);
    params.mArenaName = (*GetArenaList())[nArena].mName;
    params.mPlayMode = kPlayModeJam;
    params.mJukeboxMode = 1;
    params.mLevelName = pRecord->levelName;
    params.mLoadingGame = 1;
    CallScriptTemplate(kJukeboxStartTemplate, kJukeboxStartArgument);
    Application::shared()->GetGameManager()->SetParams(params);

    const int nAppearances = pRecord->appearances.size();
    for (int i = 0; i < nAppearances; ++i) {
        pRecord->appearances[i].AttachToBurnSlot(i);
    }

    NextTrack();
    PushNamedScreen(HxStr(kLoadGameScreen));
    ActivateNamedPanel(HxStr(kLoadGameScreen));
}

// 0x0035a6b0
void MetRemixManager::LeaveJukeboxMode() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    params.mJukeboxMode = 0;
    Application::shared()->GetGameManager()->SetParams(params);
    SetCurrentTrack(0);
}

// 0x003610d0
void MetRemixManager::PushRestoreScreens() {
    const int nScreens = mRestoreScreens.size();
    for (int i = 0; i < nScreens; ++i) {
        PushNamedScreen(mRestoreScreens[i]);
    }
    if (nScreens > 0) {
        ActivateNamedPanel(mRestoreScreens[0]);
    }
}

// 0x00361170
void MetRemixManager::PushReturnScreens() {
    const int nScreens = mReturnScreens.size();
    for (int i = 0; i < nScreens; ++i) {
        PushNamedScreen(mReturnScreens[i]);
    }
    if (nScreens > 0) {
        ActivateNamedPanel(mReturnScreens[0]);
    }
}

// 0x003612a0
void MetRemixManager::PrunePlayList() {
    mPlayList.RemoveStaleEntries();
}

// 0x00361518
void MetRemixManager::EnterAndShow() {
    mView->SetShowing(0);
}

// 0x00361550
void MetRemixManager::OnExitFinished() {
}

// 0x003553e8
void MetRemixManager::OnRemixesListed(int nPortSlot, int nStatus) {
    mListStatus[nPortSlot] = nStatus;
    ++mListingsDone;
    // Yes, the binary repeats the same exit on both sides of the status test.
    if (nStatus == kListStatusOk || nStatus == kListStatusNoRemixes) {
        if (mListingsDone == mListingsExpected && mPlayListReady != 0) {
            ExitScreenByName(HxStr(kMsgScreen));
        }
    } else if (mListingsDone == mListingsExpected && mPlayListReady != 0) {
        ExitScreenByName(HxStr(kMsgScreen));
    }
}

// 0x003563e0
void MetRemixManager::OnJukeboxPlayListLoaded([[maybe_unused]] int nPortSlot, int nStatus) {
    mPlayListReady = 1;
    // Yes, the binary repeats the same exit on both sides of the status test.
    if (nStatus != 0) {
        if (mListingsDone == mListingsExpected) {
            ExitScreenByName(HxStr(kMsgScreen));
        }
    } else if (mListingsDone == mListingsExpected) {
        ExitScreenByName(HxStr(kMsgScreen));
    }
}

// 0x00358310
void MetRemixManager::Done(int nHandle,
                           [[maybe_unused]] int nFile,
                           void *pBuffer,
                           int nLength,
                           [[maybe_unused]] int nStatus) {
    if (nHandle == mRemixRequest) {
        IOBPreallocMemStream *pLog = Application::shared()->GetResetLog();
        pLog->WriteBytes(pBuffer, nLength);
        pLog->Seek(0, kSeekFromStart);
        mRemixRequest = 0;
        if (mAfterLoadAction == kAfterLoadStart) {
            StartLoadedRemix();
        } else if (mAfterLoadAction == kAfterLoadExit) {
            ExitScreenByName(HxStr(kMsgScreen));
        }
        return;
    }

    if (nHandle != mIndexRequest) {
        return;
    }

    IOBMemStream stream;
    stream.Load(pBuffer, nLength);
    RemixIndex index;
    index.ReadFromStream(stream);
    for (std::vector<RemixIndexElement>::iterator it = index.elements.begin();
         it != index.elements.end();
         ++it) {
        MetRemixRecord record(HxStr(it->LevelName),
                              HxStr(it->RemixName),
                              HxStr(it->FileName),
                              it->dateTime,
                              it->GameOK,
                              it->appearances,
                              it->AlbumNum);
        record.factory = 1;
        mRemixes[mIndexSlot].push_back(record);
    }
    MemFreeTagged(pBuffer, __FILE__, __LINE__);
    mIndexSlot = kFactorySlot;
    mIndexRequest = 0;
    if (++mListingsDone == mListingsExpected && mPlayListReady != 0) {
        ExitScreenByName(HxStr(kMsgScreen));
    }
}

// 0x003612c0
MetRemixRecord *MetRemixManager::LookupRemix(const HxStr &name) {
    return FindRecord(name);
}

// 0x00361418
inline void MetRemixManager::LoadRemix(const MetRemixRecord &record, int nFactory) {
    if (nFactory != 0) {
        LoadRemixFile(record.fileName);
        return;
    }
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateLoadRemixTask(0, record.name);
}

// 0x00361480
void MetRemixManager::LoadCurrentTrack() {
    mAfterLoadAction = 0;
    JukeboxPlayListEntry *pEntry = mPlayList.GetEntry(mCurrentPlaylistTrack);
    LoadRemix(*FindRecord(pEntry->name), pEntry->factory);
}

// 0x003612e0
void MetRemixManager::PlayCurrentTrack() {
    LoadCurrentTrack();
}
