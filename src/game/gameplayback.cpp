#include "game/gameplayback.h"

#include "app/application.h"
#include "app/scheduler.h"
#include "game/gamemanagerimpl.h"
#include "msg/hxstrtransfer.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "stream/ibfilestream.h"

// NTSC-U/C: 0x0010cf30, PAL: 0x0010d208
GamePlayback::GamePlayback(const HxStr &file, GameManagerImpl *pManager, int) : mManager(pManager) {
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

// NTSC-U/C: 0x0010f0d8, PAL: 0x0010f538
GamePlayback::~GamePlayback() {
    Application::shared()->GetWatchdog()->Close();
}
