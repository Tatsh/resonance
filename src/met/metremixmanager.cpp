#include "met/metremixmanager.h"

#include <algorithm>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "memcard/memcardmanager.h"
#include "memcard/remixindex.h"
#include "met/metsonglists.h"
#include "os/async.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/r250.h"
#include "os/zone.h"
#include "script/scripthost.h"
#include "stream/iobmemstream.h"
#include "stream/iobpreallocmemstream.h"

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

// What Done() does once a remix read completes, as mUnknownf4 records it.
constexpr int kAfterLoadStart = 0;
constexpr int kAfterLoadExit = 1;

// The origin Done() rewinds the reset log to.
constexpr int kSeekFromStart = 0;

// The screens StartPlayList() records for the return from a jukebox game.
constexpr char kTopButtonsScreen[] = "MetJukeboxTopButtonsScreen";
constexpr char kHelpScreen[] = "MetHelpScreen";

// The screen StartLoadedRemix() pushes and activates.
constexpr char kLoadGameScreen[] = "MetLoadGameScreen";

// The script template StartLoadedRemix() runs, and the one argument it passes.
constexpr int kJukeboxStartTemplate = 0x267;
constexpr char kJukeboxStartArgument[] = "1";

// 0x006c1100
HxStr g_remixIndexPath("Levels/remixes/ps2/index");

// 0x006c1108
HxStr g_remixDirectory("Levels/remixes/ps2/");

inline const char *PathOrEmpty(const HxStr &path) {
    return path.mStr != nullptr ? path.mStr : g_szEmptyString;
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

inline void MetRemixManager::SetReturnScreens(const std::vector<HxStr> &screens) {
    mUnknownac.clear();
    mUnknownac.resize(screens.size());
    mUnknownac = screens;
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
    SetReturnScreens(returnScreens);
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
