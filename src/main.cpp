#include <iostream>

#include "app/application.h"
#include "gfx/gfxdevice.h"
#include "os/arkfile.h"
#include "os/async.h"
#include "os/failsink.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/iop.h"
#include "os/log.h"
#include "os/zone.h"
#include "rnd/asyncloader.h"
#include "rnd/manager.h"
#include "rnd/view.h"

#ifdef ENABLE_PATCHES
#include "os/bootlog.h"
#endif

namespace {

// The loading screen has its own archive, which is mounted before the session archives and
// unmounted once the screen has been drawn.
constexpr char kLoadingArkPath[] = "ark/loading.ark";

constexpr int kDisplayWidth = 640;
constexpr int kDisplayHeight = 448;
constexpr int kDisplayBitDepth = 16;

// A newline follows every kLoadingDotsPerLine progress dots.
constexpr int kLoadingDotsPerLine = 64;

int g_nLoadingDots = 0;

const char *g_szLastFailure;

// 0x001f2638
void RecordFailMessage(const char *pszMessage) {
    std::cout << pszMessage;
    g_szLastFailure = pszMessage;
}

// 0x001f2670
void HaltOnFailure() {
    Fatal(g_szLastFailure);
}

// 0x001f2698
// ShowLoadingScreen expands this inline; the out-of-line copy has no caller.
inline void PrintLoadingDot() {
    ++g_nLoadingDots;
    LogPrintf(".%s", (g_nLoadingDots & (kLoadingDotsPerLine - 1)) == 0 ? "\n" : "");
}

// 0x001ef620
void ShowLoadingScreen() {
    RndAsyncLoader loader(HxStr("loading/"), HxStr("loading.rnd"), -1);
    loader.Enqueue();

    float flProgress = 0.0f;
    while (loader.Poll(&flProgress) == 0) {
        PrintLoadingDot();
        RndAsyncLoader::PollAsyncLoads();
    }

    // The progress the poll reports is discarded; the screen is resolved by name instead. The
    // binary really does dispatch this through the runtime cast helper.
    Rnd::View *pView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("view")));
    g_gfxDevice.BeginFrame();
    pView->Draw();
    g_gfxDevice.PresentFrame(1);
}

} // namespace

// 0x001ef870
int main() {
    LogPrintf("\n\n**********************\n");
    LogPrintf("FREQ session beginning\n");
    LogPrintf("**********************\n");

    SetZonesEnabled(1);
    InitIop();
#ifdef ENABLE_PATCHES
    BootLogOpen();
    BootLogCheckpoint("IOP rebooted");
#endif
    InitAsync();
#ifdef ENABLE_PATCHES
    BootLogCheckpoint("async file layer started");
#endif

    g_failSink.SetReportHandler(RecordFailMessage);
    g_failSink.mAbortProc = HaltOnFailure;

    Rnd::g_manager.Init();
#ifdef ENABLE_PATCHES
    BootLogCheckpoint("renderer manager started");
#endif
    g_gfxDevice.Init(kDisplayWidth, kDisplayHeight, kDisplayBitDepth);
#ifdef ENABLE_PATCHES
    BootLogCheckpoint("GS started at %dx%d, %d bits, NTSC interlaced",
                      kDisplayWidth,
                      kDisplayHeight,
                      kDisplayBitDepth);
#endif

    if (UsingArkFiles() != 0) {
        if (ArkFile::Open(kLoadingArkPath) == 0) {
            Fatal("Can't open loading.ark arkfile!\n");
        }
    }

    ShowLoadingScreen();
#ifdef ENABLE_PATCHES
    BootLogCheckpoint("loading screen drawn");
#endif

    if (UsingArkFiles() != 0) {
        ArkFile::Close(kLoadingArkPath);
    }

    InitArk(); // Yes, the binary discards this call's result.
#ifdef ENABLE_PATCHES
    BootLogCheckpoint("session archives opened");
#endif
    LoadIopModules();
#ifdef ENABLE_PATCHES
    BootLogCheckpoint("IOP modules loaded");
#endif

    Application::shared()->Run(); // Yes, the binary discards this call's result.
    return 0;
}
