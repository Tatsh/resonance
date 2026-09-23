#include "met/metremixmanager.h"

#include <algorithm>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "memcard/memcardmanager.h"
#include "os/async.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/r250.h"
#include "os/zone.h"

namespace {

// The screen name, directory, and container the manager registers under.
constexpr char kScreenName[] = "dlg";
constexpr char kDirectory[] = "metagame/Shared";
constexpr char kContainerName[] = "dialogue";

// The registry key shared() resolves the instance by.
constexpr char kRegistryName[] = "MetRemixManager";

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
void MetRemixManager::NextTrack() {
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

// 0x0035a6b0
void MetRemixManager::LeaveJukeboxMode() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    params.mJukeboxMode = false;
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
