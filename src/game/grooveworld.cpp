#include "game/grooveworld.h"

#include <iostream>
#include <vector>

#include "app/application.h"
#include "app/renderer.h"
#include "game/delayer.h"
#include "game/gamemanagerimpl.h"
#include "game/gamestats.h"
#include "game/inputmap.h"
#include "game/levelbuilder.h"
#include "game/levelconverter.h"
#include "game/leveldata.h"
#include "game/netplayer.h"
#include "game/phrasedatabase.h"
#include "game/player.h"
#include "game/scoretrackgraph.h"
#include "game/trackselector.h"
#include "msg/bumppacket.h"
#include "msg/cripplepacket.h"
#include "msg/message.h"
#include "os/async.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"
#include "sch/command.h"
#include "sch/tickclock.h"
#include "script/configquery.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace {

// Configuration identifiers the load path queries.
constexpr int kTrackCountQuery = 0x384;
constexpr int kLevelConverterOptionQuery = 0x39a;

// mState values.
constexpr int kStateLoading = 1;
constexpr int kStateLoaded = 2;

// AsyncPollComplete() results.
constexpr int kAsyncComplete = 0;
constexpr int kAsyncPending = -1;

// The tag and line FinishLoad() bills the release of the file buffer to.
constexpr char kAllocTag[] = "GrooveWorld.cpp";
constexpr int kAllocLine = 302;

// Exit modes PostExitMode1(), PostExitMode2(), and PostExitMode3() queue.
constexpr int kExitMode1 = 1;
constexpr int kExitMode2 = 2;
constexpr int kExitMode3 = 3;

/**
 * Scheduler command that calls one member of the world.
 *
 * `Q234_GLOBAL_$N$_13ControllerCmd$sCmdID7FuncCmd` in the RTTI, with Sch::Command as its one base.
 * The anonymous-namespace marker records this translation unit through its first global,
 * ControllerCmd::sCmdID. Its vtable at `0x007dc378` retains Sch::Command::Save() and Load(). The
 * GrooveWorld routines that queue one allocate 0x18 bytes and expand the constructor inline,
 * storing the world at `+0x0c` and an eight-byte pointer to member function at `+0x10`.
 *
 * The destructor at `0x001947d8` is implicitly declared. It stores the base table pointer and runs
 * Attachment's destructor, which is what the compiler generates.
 */
class FuncCmd : public Sch::Command {
public:
    FuncCmd(GrooveWorld *pWorld, void (GrooveWorld::*pfnFunc)()) : mWorld(pWorld), mFunc(pfnFunc) {
    }

    // 0x00194850
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x00194860
    virtual void Execute() {
        (mWorld->*mFunc)();
    }

    // 0x001948e0
    virtual void Print(std::ostream &stream) {
        stream << "{GWFunc}";
    }

    // The word at 0x0067f244, which the image initialises to zero.
    static int sCmdID;

private:
    GrooveWorld *mWorld;          // +0x0c
    void (GrooveWorld::*mFunc)(); // +0x10
};

int FuncCmd::sCmdID;

/**
 * Scheduler command that makes the world leave the game.
 *
 * `Q234_GLOBAL_$N$_13ControllerCmd$sCmdID7ExitCmd` in the RTTI, with Sch::Command as its one base,
 * in the same translation unit as FuncCmd. Its vtable at `0x007dc330` overrides every slot the
 * base declares apart from Attachment::Destroy(). Both constructors have out-of-line copies and no
 * caller in the image, and the GrooveWorld routine at `0x0018e368` expands the second inline.
 *
 * The destructor at `0x00194910` is implicitly declared, for the reason recorded on FuncCmd.
 */
class ExitCmd : public Sch::Command {
public:
    // 0x00194998
    ExitCmd() {
    }

    // 0x001949b8
    ExitCmd(int nMode, int nUnknown10, int nUnknown14)
        : mMode(nMode), mUnknown10(nUnknown10), mUnknown14(nUnknown14) {
    }

    // 0x00194988
    virtual int CmdID() {
        return sCmdID;
    }

    // 0x001949e8
    virtual void Execute() {
        Application::shared()->GetWorld()->Exit(mMode, mUnknown10, mUnknown14);
    }

    // 0x00194a28
    virtual void Print(std::ostream &stream) {
        stream << "{ExitCmd}";
    }

    // 0x00194a50
    virtual void Save(OBStream &stream) {
        const int nMode = mMode;
        stream.Write(&nMode, sizeof(nMode));
        stream << mUnknown10;
        stream << mUnknown14;
    }

    // 0x00194ab8
    virtual void Load(IBStream &stream) {
        int nMode;
        stream.Read(&nMode, sizeof(nMode));
        mMode = nMode;
        stream >> mUnknown10;
        stream >> mUnknown14;
    }

    // The word at 0x0067f24c, which the image initialises to 7.
    static int sCmdID;

private:
    int mMode;      // +0x0c
    int mUnknown10; // +0x10, one byte on the wire
    int mUnknown14; // +0x14, one byte on the wire
};

constexpr int kExitCmdId = 7;

int ExitCmd::sCmdID = kExitCmdId;

} // namespace

// 0x00195388
void GrooveWorld::HandleMessage(Message *pMsg) {
    const int nType = pMsg->Type();
    if (nType == g_nCripplePacketType) {
        OnCripplePacket(pMsg);
    } else if (nType == g_nBumpPacketType) {
        OnBumpPacket(pMsg);
    }
}

// 0x00195348
void GrooveWorld::OnCripplePacket(Message *pMsg) {
    mDelayer->Handle(pMsg);
}

// 0x00194b50
void GrooveWorld::OnBumpPacket(Message *pMsg) {
    mTrackSelector->Handle(pMsg);
}

// 0x00194b80
void GrooveWorld::SetUnknown20And1c(MsgSink *pSink, MsgSource *pSource) {
    mUnknown20 = pSink;
    if (mApp->GetGameMode() == kGameModeNet) {
        mUnknown1c = pSource;
    } else {
        mUnknown1c = nullptr;
    }
}

// 0x00194bc8
void GrooveWorld::StartLoad(const HxStr &path) {
    mLevel = new LevelBuilder(QueryConfigValue(kTrackCountQuery));
    mLevelPath = path;

    const int nZone = ZoneGetCurrent();
    ZoneSetCurrent(kNoZone);
    mLoadHandle = AsyncLoadFileByPath(
        path.mStr != nullptr ? path.mStr : g_szEmptyString, nullptr, 0, nullptr);
    ZoneSetCurrent(nZone);

    mState = kStateLoading;
}

// 0x00194ca0
int GrooveWorld::IsLoadDone() {
    AsyncPumpCompletedRequests();
    const int nResult = AsyncPollComplete(mLoadHandle, &mLoadBuffer, &mLoadSize);
    if (nResult == kAsyncComplete) {
        return 1;
    }
    if (nResult == kAsyncPending) {
        return 0;
    }
    Fatal("Error reading midi file asynchronously\n");
    return 0;
}

// 0x00194d00
void GrooveWorld::FinishLoad() {
    LevelConverter converter;
    if (QueryConfigFlag(kLevelConverterOptionQuery) != 0) {
        converter.mUnknown90 = 1;
    }
    converter.Convert(mLevelPath.mStr != nullptr ? mLevelPath.mStr : g_szEmptyString,
                      mLoadBuffer,
                      mLoadSize,
                      mLevel);
    MemFreeTagged(mLoadBuffer, kAllocTag, kAllocLine);

    mSongClock->SetTempoMap(mLevel->OnUnknownSlot7());

    mLoadHandle = 0;
    mState = kStateLoaded;
    mLoadBuffer = nullptr;
    mLoadSize = 0;
}

// 0x00194de8
void GrooveWorld::AddNetPlayer(int nId,
                               [[maybe_unused]] int nUnused,
                               const HxStr &name,
                               int nUnknown2c) {
    (void)(name != ""); // Yes, the binary discards this comparison's result.
    Player *pPlayer = new NetPlayer(nId, nId, name, nUnknown2c);
    mPlayers.push_back(pPlayer);
}

// 0x00194ef0
void GrooveWorld::RemovePlayer(int nId) {
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        if ((*it)->mId20 == nId) {
            mPlayers.erase(it);
            return;
        }
    }
}

// 0x00194f88
void GrooveWorld::DestroyRenderer() {
    if (mDelayer != nullptr) {
        mDelayer->RemoveSink(GetRendererSink());
    }
    for (std::vector<Player *>::iterator it = mPlayers.begin(); it != mPlayers.end(); ++it) {
        (*it)->RemoveSink(GetRendererSink());
    }
    delete mRenderer;
    mRenderer = nullptr;
}

// 0x00195058
void GrooveWorld::SavePhrases(OBStream &stream) {
    for (std::vector<ScoreTrackGraph *>::iterator it = mTrackGraphs.begin();
         it != mTrackGraphs.end();
         ++it) {
        (*it)->GetPhraseDatabase()->Save(stream);
    }
}

// 0x001950c0
void GrooveWorld::LoadPhrases(IBStream &stream, int bClearOwners) {
    for (std::vector<ScoreTrackGraph *>::iterator it = mTrackGraphs.begin();
         it != mTrackGraphs.end();
         ++it) {
        PhraseDatabase *pDatabase = (*it)->GetPhraseDatabase();
        pDatabase->Load(stream);
        if (bClearOwners != 0) {
            pDatabase->ClearOwners();
        }
    }
}

// 0x00195150
void GrooveWorld::EnableInput() {
    mInputMap->EnableEntries();
}

// 0x00195170
void GrooveWorld::PostExitMode1() {
    PostExit(kExitMode1, mUnknownb8, 0);
}

// 0x00195198
void GrooveWorld::PostExitMode2() {
    PostExit(kExitMode2, 0, 0);
}

// 0x001951c0
void GrooveWorld::PostExitMode3() {
    PostExit(kExitMode3, 0, 1);
}

// 0x001952a0
Sch::TickClock *GrooveWorld::GetSongClock() {
    return mSongClock;
}

// 0x001952a8
PlayMap *GrooveWorld::GetPlayMap() {
    return mLevel->OnUnknownSlot8();
}

// 0x001952d8
LevelData *GrooveWorld::GetLevel() {
    return mLevel;
}

// 0x001952e0
MsgSink *GrooveWorld::GetRendererSink() {
    return mRenderer;
}

// 0x00195378
void GrooveWorld::MarkStatsFlag() {
    mStats->mUnknown14 = 1;
}
