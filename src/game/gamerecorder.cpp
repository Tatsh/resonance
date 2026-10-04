#include "game/gamerecorder.h"

#include "app/application.h"
#include "app/scheduler.h"
#include "app/timeclock.h"
#include "game/endrecordingcmd.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "sch/cmdid.h"
#include "sch/time.h"
#include "stream/obfilestream.h"

namespace {

// The handle a post starts with, before the scheduler allocates one.
constexpr int kUnallocatedCommand = -2;

// The end of a recording is itself recorded.
constexpr int kRecordable = 1;

// Values of GameParams::mDifficulty that BeginRecording() describes by name.
constexpr int kDifficultyEasy = 0;
constexpr int kDifficultyMedium = 1;

// The length-prefixed string form the recording header uses. The binary expands it at each use.
inline void WriteText(OBStream &stream, const HxStr &text) {
    unsigned length = text.mLen;
    stream.WriteLE(&length, sizeof(length));
    stream.Write(text.mStr != nullptr ? text.mStr : g_szEmptyString, length);
}

} // namespace

void GameRecorder::BeginRecording(int nGameMode, const GameParams &params) {
    const HxStr path = MakeFreqPath(HxStr("rec.bin"));
    mStream = new OBFileStream(path);

    HxStr banner;
    banner += "PS2 application...  Unknown size and modification time\n Details below: ";
    WriteText(*mStream, banner);

    HxStr description;
    const char *pszMode;
    switch (nGameMode) {
    case kGameModeSolo:
        pszMode = " (solo ";
        break;
    case kGameModeLocal:
        pszMode = " (local ";
        break;
    default:
        pszMode = " (net ";
        break;
    }
    const char *pszPlay = params.mPlayMode == kPlayModeGame ? "game " : "jam ";
    HxStr difficulty;
    switch (params.mDifficulty) {
    case kDifficultyEasy:
        difficulty = "easy)\n";
        break;
    case kDifficultyMedium:
        difficulty = "medium)\n";
        break;
    default:
        difficulty = "hard)\n";
        break;
    }
    description += params.mLevelName + pszMode;
    description += pszPlay;
    description += difficulty;
    WriteText(*mStream, description);

    HxStr noAutoexec("no autoexec");
    WriteText(*mStream, noAutoexec);
    mManager->Save(mStream);
    Application::shared()->GetWatchdog()->BeginRecording(*mStream);
}

GameRecorder::GameRecorder(GameManagerImpl *pManager) : mManager(pManager), mStream(nullptr) {
}

GameRecorder::~GameRecorder() {
    delete mStream;
}

void GameRecorder::ScheduleEnd() {
    EndRecordingCmd *pCommand = new EndRecordingCmd(this);
    Sch::CmdID id;
    id.mValue = kUnallocatedCommand;
    const Sch::Time now{0};
    Application::shared()->GetWatchdogTimer()->PostIn(pCommand, now, id, kRecordable);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

void GameRecorder::FinishUp() {
    Application::shared()->GetWatchdog()->StopRecOrPlayback();
    delete mStream;
    mStream = nullptr;
}
