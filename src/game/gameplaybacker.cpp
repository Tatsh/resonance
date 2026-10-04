#include "game/gameplaybacker.h"

#include "app/application.h"
#include "app/scheduler.h"
#include "game/gamemanagerimpl.h"
#include "msg/hxstrtransfer.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "stream/ibfilestream.h"

GamePlaybacker::GamePlaybacker(const HxStr &file, GameManagerImpl *pManager, int)
    : mManager(pManager) {
    IBFileStream stream(MakeFreqPath(file));

    // Yes, the binary reads the first two strings into the same string and discards all three.
    HxStr first;
    LoadHxStr(stream, first);
    LoadHxStr(stream, first);
    HxStr third;
    LoadHxStr(stream, third);

    mManager->Load(&stream);
    Application::shared()->GetWatchdog()->StartPlayback(stream);
}

GamePlaybacker::~GamePlaybacker() {
    Application::shared()->GetWatchdog()->StopRecOrPlayback();
}
