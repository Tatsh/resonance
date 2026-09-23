#include "met/metremixmanager.h"

#include <algorithm>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/globalsettings.h"
#include "memcard/memcardmanager.h"
#include "memcard/remixindex.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "met/metsonglists.h"
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

// What Done() does once a remix read completes, as mUnknownf4 records it.
constexpr int kAfterLoadStart = 0;
constexpr int kAfterLoadExit = 1;

// The origin Done() rewinds the reset log to.
constexpr int kSeekFromStart = 0;

// The configuration code the dialogue texts are read under, and the code of the playlist number
// the save and load tasks carry.
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

// A dialogue text read by value from configuration.
inline HxStr ConfigText(const char *pszKey) {
    HxStr value;
    QueryConfigString(&value, kDialogueConfigCode, pszKey);
    return value;
}

// The display name of the first memory-card slot, or the empty string.
inline const char *FirstCardSlotText() {
    return PathOrEmpty(GlobalSettings::shared()->mCardSlots[0].mName);
}

// Replaces a list of screen names by clearing, resizing, and then assigning, which is the sequence
// every writer of mUnknownac and mUnknownb8 expands.
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
      mUnknownd4(0), mUnknownd8(0), mUnknowndc(1), mIndexRequest(0), mRemixRequest(0),
      mUnknowne8(-1), mCurrentPlaylistTrack(0), mShuffle(0), mUnknownf4(0) {
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
            if (!(records[i].unknown08_ == name)) {
                continue;
            }
            if (records[i].unknown24_ != 0) {
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
    LogPrintf("MetRemixManager::RandomTrack() - mCurrentPlaylistTrack = %i.\n",
              mCurrentPlaylistTrack);
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

// 0x00353350
void MetRemixManager::ListRemixes(const std::vector<HxStr> &returnScreens,
                                  std::vector<CardSlot> slots,
                                  int bLoadPlayList) {
    CacheSharedInstance(); // Yes, the binary resolves its own instance first and ignores it.
    ReplaceScreens(mUnknownac, returnScreens);
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
        factoryText = ConfigText(kFactoryListLoadText);
        nParts = 1;
    }
    if (cardSlots.size() != 0) {
        ++nParts;
        slotNames = slots[cardSlots[0]].mName;
        for (unsigned i = 1; i < cardSlots.size(); ++i) {
            slotNames.Insert(slotNames.mLen, HxStr(kSlotNameSeparator));
            slotNames.Insert(slotNames.mLen, slots[cardSlots[i]].mName);
        }
        cardText = FormatString(PathOrEmpty(ConfigText(kMemLoadDialogue)), PathOrEmpty(slotNames));
    }
    if (nParts == kBothSources) {
        text = FormatString(kBothSourcesFormat, PathOrEmpty(factoryText), PathOrEmpty(cardText));
    } else if (bHasFactory != 0) {
        text = factoryText;
    } else {
        text = cardText;
    }
    MetMsgScreen::Show(HxStr(kMemLoadDialogue), HxStr(kWarningTitle), text, 0, buttons, this);

    MemcardManager::shared()->mUser = this;
    mRemixes.clear();
    mListStatus.clear();
    mUnknownd4 = 0;
    mCurrentPlaylistTrack = 0;
    const int nSlots = slots.size();
    mUnknownd8 = nSlots;
    for (int i = 0; i < nSlots; ++i) {
        if (slots[i].mPortSlot == kFactorySlot) {
            mUnknowne8 = slots[i].mPortSlot;
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
        mUnknowndc = 0;
        MemcardManager::shared()->CreateLoadJukeboxPlayListTask(
            0, &mPlayList, QueryConfigValue(kPlayListIndexCode));
    }
}

// 0x003548e8
void MetRemixManager::BeginRemixLoad(const std::vector<HxStr> &returnScreens,
                                     const std::vector<HxStr> &restoreScreens,
                                     const MetRemixRecord &record,
                                     int nFactory) {
    const std::vector<HxStr> buttons;
    HxStr text;
    if (nFactory != 0) {
        if (Application::shared()->GetPlayMode() == kPlayModeGame) {
            text = ConfigText(kFactoryCustomLoadText);
        } else {
            text = ConfigText(kFactoryRemixLoadText);
        }
    } else {
        const HxStr slotName = FirstCardSlotName();
        HxStr format;
        if (Application::shared()->GetPlayMode() == kPlayModeGame) {
            format = ConfigText(kCustomLoadText);
        } else {
            format = ConfigText(kRemixLoadText);
        }
        text = FormatString(PathOrEmpty(format), PathOrEmpty(slotName));
    }
    MetMsgScreen::Show(HxStr(kLoadRemixDataDialogue), HxStr(kWarningTitle), text, 0, buttons, this);
    mUnknownf4 = kAfterLoadExit;
    ReplaceScreens(mUnknownac, returnScreens);
    ReplaceScreens(mUnknownb8, restoreScreens);
    LoadRemix(record, nFactory);
}

// 0x00355ff0
void MetRemixManager::OnRemixLoaded(int nPortSlot, int nStatus) {
    LogPrintf(" in MetRemixManager::LoadRemixCB(). Return code = %i.\n", nStatus);
    if (nStatus == 0) {
        if (mUnknownf4 == kAfterLoadStart) {
            StartLoadedRemix();
        } else if (mUnknownf4 == kAfterLoadExit) {
            ExitScreenByName(HxStr(kMsgScreen));
        }
        return;
    }

    LogPrintf("Failed to load remix from memory card slot %i.", nPortSlot);
    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kOkButton));
    const HxStr name(kRemixLoadFailedDialogue);
    const HxStr title(kWarningTitle);
    HxStr text;
    QueryConfigString(&text, kDialogueConfigCode, kLoadFailText);
    MetMsgScreen::Show(name, title, text, 1, buttons, this);
}

inline void MetRemixManager::RetrySavePlayList() {
    std::vector<HxStr> screens;
    screens = mUnknownac;
    SavePlayList(screens);
}

// 0x003555c0
void MetRemixManager::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kLoadRemixDataDialogue) {
        PushUnknownacScreens();
    } else if (name == kMemLoadDialogue) {
        PushUnknownacScreens();
        mUnknownd8 = -1;
        mUnknownd4 = -1;
    } else if (name == kMemSaveDialogue || name == kPlayListSaveFailedDialogue) {
        PushUnknownacScreens();
    } else if (name == kPlayListSaveRetryDialogue) {
        if (nChoice != kChoiceFirst) {
            PushUnknownacScreens();
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
            mUnknown10->OnUnknownSlot5();
            MetFreqEndedMsg msg;
            msg.mUnknownb8Clear = params.mJukeboxMode;
            mUnknown10->Handle(&msg);
        } else {
            PushUnknownb8Screens();
        }
    } else if (name == kFormatCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->CreateFormatTask(0);
            const std::vector<HxStr> buttons;
            GlobalSettings::shared(); // Yes, the binary discards this call's result.
            const HxStr format(ConfigText(kFormatGoDialogue));
            const HxStr text(FormatString(PathOrEmpty(format), FirstCardSlotText()));
            MetMsgScreen::Show(
                HxStr(kFormatGoDialogue), HxStr(kWarningTitle), text, 0, buttons, this);
        } else {
            RetrySavePlayList();
        }
    } else if (name == kFormatDoneDialogue) {
        RetrySavePlayList();
    } else if (name == kFormatFailDialogue) {
        if (nChoice != kChoiceFirst) {
            PushUnknownacScreens();
        } else {
            RetrySavePlayList();
        }
    }
}

// 0x003569d0
void MetRemixManager::OnCardFormatted([[maybe_unused]] int nPortSlot, int nStatus) {
    if (nStatus == 0) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kContinueButton));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr format(ConfigText(kFormatSuccessText));
        const HxStr text(FormatString(PathOrEmpty(format), FirstCardSlotText()));
        MetMsgScreen::ShowActive(
            HxStr(kFormatDoneDialogue), HxStr(kFormatTitle), text, kOneButton, buttons, this);
    } else if (nStatus == kCardStatusAlreadyFormatted) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kContinueButton));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr format(ConfigText(kFormatAlreadyText));
        const HxStr text(FormatString(PathOrEmpty(format), FirstCardSlotText()));
        MetMsgScreen::ShowActive(
            HxStr(kFormatDoneDialogue), HxStr(kFormatTitle), text, kOneButton, buttons, this);
    } else {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        MetMsgScreen::Show(HxStr(kFormatFailDialogue),
                           HxStr(kErrorTitle),
                           ConfigText(kFormatFailDialogue),
                           kTwoButtons,
                           buttons,
                           this);
    }
}

// 0x003573e8
void MetRemixManager::OnJukeboxPlayListSaved([[maybe_unused]] int nPortSlot, int nStatus) {
    std::vector<HxStr> buttons;
    HxStr format;
    HxStr text;
    switch (nStatus) {
    case 0:
        ExitScreenByName(HxStr(kMsgScreen));
        break;

    case kCardStatusUnformatted:
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        format = ConfigText(kFormatCheckDialogue);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        text = FormatString(PathOrEmpty(format), FirstCardSlotText());
        MetMsgScreen::Show(
            HxStr(kFormatCheckDialogue), HxStr(kSettingsTitle), text, kTwoButtons, buttons, this);
        break;

    case kCardStatusNoCard:
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        format = ConfigText(kSaveFailNoCardText);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        text = FormatString(PathOrEmpty(format), FirstCardSlotText());
        MetMsgScreen::Show(HxStr(kPlayListSaveRetryDialogue),
                           HxStr(kErrorTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        break;

    case kCardStatusNoSpace:
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        format = ConfigText(kSaveFailNoSpaceText);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        text = FormatString(PathOrEmpty(format), FirstCardSlotText());
        MetMsgScreen::Show(HxStr(kPlayListSaveRetryDialogue),
                           HxStr(kErrorTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        break;

    default:
        buttons.push_back(HxStr(kOkButton));
        MetMsgScreen::Show(HxStr(kPlayListSaveFailedDialogue),
                           HxStr(kErrorTitle),
                           ConfigText(kSaveFailText),
                           kOneButton,
                           buttons,
                           this);
        break;
    }
}

// 0x00356508
void MetRemixManager::SavePlayList(const std::vector<HxStr> &returnScreens) {
    ReplaceScreens(mUnknownac, returnScreens);
    const std::vector<HxStr> buttons;
    HxStr format;
    QueryConfigString(&format, kDialogueConfigCode, kMemSaveDialogue);
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    const HxStr text(FormatString(PathOrEmpty(format), FirstCardSlotText()));
    MetMsgScreen::Show(HxStr(kMemSaveDialogue), HxStr(kWarningTitle), text, 0, buttons, this);
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateSaveJukeboxPlayListTask(
        0, &mPlayList, QueryConfigValue(kPlayListIndexCode));
}

// 0x003593d0
void MetRemixManager::StartPlayList(const std::vector<HxStr> &returnScreens, int nShuffle) {
    mUnknownb8.clear();
    mUnknownb8.push_back(HxStr(kTopButtonsScreen));
    mUnknownb8.push_back(HxStr(kHelpScreen));
    mPlayedTracks.resize(mPlayList.entries.size());
    std::fill(mPlayedTracks.begin(), mPlayedTracks.end(), false);
    SetCurrentTrack(0);
    mShuffle = nShuffle;
    if (nShuffle != 0) {
        RandomTrack();
    }
    ReplaceScreens(mUnknownac, returnScreens);
    LoadCurrentTrack();
}

// 0x0035abf8
void MetRemixManager::StartLoadedRemix() {
    MetRemixRecord *pRecord = FindRecord(mPlayList.GetEntry(mCurrentPlaylistTrack)->name);
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    const int nArena = RandomInt(0, GetArenaList()->size() - 1);
    params.mArenaName = (*GetArenaList())[nArena].mName;
    params.mUnknown1c = kPlayModeJam;
    params.mJukeboxMode = 1;
    params.mLevelName = pRecord->unknown00_;
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
void MetRemixManager::PushUnknownb8Screens() {
    const int nScreens = mUnknownb8.size();
    for (int i = 0; i < nScreens; ++i) {
        PushNamedScreen(mUnknownb8[i]);
    }
    if (nScreens > 0) {
        ActivateNamedPanel(mUnknownb8[0]);
    }
}

// 0x00361170
void MetRemixManager::PushUnknownacScreens() {
    const int nScreens = mUnknownac.size();
    for (int i = 0; i < nScreens; ++i) {
        PushNamedScreen(mUnknownac[i]);
    }
    if (nScreens > 0) {
        ActivateNamedPanel(mUnknownac[0]);
    }
}

// 0x003612a0
void MetRemixManager::PrunePlayList() {
    mPlayList.RemoveUnknownEntries();
}

// 0x00361518
void MetRemixManager::EnterAndShow() {
    mUnknown14->SetShowing(0);
}

// 0x003553e8
void MetRemixManager::OnRemixesListed(int nPortSlot, int nStatus) {
    mListStatus[nPortSlot] = nStatus;
    ++mUnknownd4;
    // Yes, the binary repeats the same exit on both sides of the status test.
    if (nStatus == kListStatusOk || nStatus == kListStatusNoRemixes) {
        if (mUnknownd4 == mUnknownd8 && mUnknowndc != 0) {
            ExitScreenByName(HxStr(kMsgScreen));
        }
    } else if (mUnknownd4 == mUnknownd8 && mUnknowndc != 0) {
        ExitScreenByName(HxStr(kMsgScreen));
    }
}

// 0x003563e0
void MetRemixManager::OnJukeboxPlayListLoaded([[maybe_unused]] int nPortSlot, int nStatus) {
    mUnknowndc = 1;
    // Yes, the binary repeats the same exit on both sides of the status test.
    if (nStatus != 0) {
        if (mUnknownd4 == mUnknownd8) {
            ExitScreenByName(HxStr(kMsgScreen));
        }
    } else if (mUnknownd4 == mUnknownd8) {
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
        if (mUnknownf4 == kAfterLoadStart) {
            StartLoadedRemix();
        } else if (mUnknownf4 == kAfterLoadExit) {
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
        record.unknown24_ = 1;
        mRemixes[mUnknowne8].push_back(record);
    }
    MemFreeTagged(pBuffer, __FILE__, __LINE__);
    mUnknowne8 = kFactorySlot;
    mIndexRequest = 0;
    if (++mUnknownd4 == mUnknownd8 && mUnknowndc != 0) {
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
        LoadRemixFile(record.unknown10_);
        return;
    }
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateLoadRemixTask(0, record.unknown08_);
}

// 0x00361480
void MetRemixManager::LoadCurrentTrack() {
    mUnknownf4 = 0;
    JukeboxPlayListEntry *pEntry = mPlayList.GetEntry(mCurrentPlaylistTrack);
    LoadRemix(*FindRecord(pEntry->name), pEntry->factory);
}

// 0x003612e0
void MetRemixManager::PlayCurrentTrack() {
    LoadCurrentTrack();
}
