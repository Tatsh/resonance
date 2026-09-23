#include "game/gamerecorder.h"

#include "app/application.h"
#include "app/watchdog.h"
#include "app/watchdogtimer.h"
#include "game/endrecordingcmd.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "sch/cmdid.h"
#include "sch/tick.h"
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
    stream.Write(&length, sizeof(length));
    stream.WriteBytes(text.mStr != nullptr ? text.mStr : g_szEmptyString, length);
}

} // namespace

// 0x0010c910
void GameRecorder::BeginRecording(int nGameMode, const GameParams &params) {
    const HxStr path = MakeFreqPath(HxStr("rec.bin"));
    mStream = new OBFileStream(path);

    HxStr banner;
    banner += "PS2 application.";
    WriteText(*mStream, banner);

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
    const char *pszPlay = params.mUnknown1c == kPlayModeGame ? "game " : "jam ";
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
    HxStr description;
    description += params.mLevelName + pszMode;
    description += pszPlay;
    description += difficulty;
    WriteText(*mStream, description);

    WriteText(*mStream, HxStr("no autoexec"));
    mManager->Save(mStream);
    Application::shared()->GetWatchdog()->BeginRecording(*mStream);
}

// 0x0010f010
GameRecorder::GameRecorder(GameManagerImpl *pManager) : mManager(pManager), mStream(nullptr) {
}

// 0x0010f020
GameRecorder::~GameRecorder() {
    delete mStream;
}

// 0x0010cea0
void GameRecorder::ScheduleEnd() {
    EndRecordingCmd *pCommand = new EndRecordingCmd(this);
    CmdID id;
    id.mValue = kUnallocatedCommand;
    const Sch::Tick now{0};
    Application::shared()->GetWatchdogTimer()->PostIn(pCommand, now, id, kRecordable);
    if (pCommand != nullptr) {
        pCommand->Release();
    }
}

// 0x0010f080
void GameRecorder::EndRecording() {
    Application::shared()->GetWatchdog()->Close();
    delete mStream;
    mStream = nullptr;
}
