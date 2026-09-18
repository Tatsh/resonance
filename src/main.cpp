#include "app/application.h"
#include "gfx/gfxdevice.h"
#include "os/arkfile.h"
#include "os/async.h"
#include "os/debugstream.h"
#include "os/failsink.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/iop.h"
#include "os/log.h"
#include "os/zone.h"
#include "rnd/asyncloader.h"
#include "rnd/manager.h"
#include "rnd/view.h"

namespace {

// The loading screen has its own archive, which is mounted before the session archives and
// unmounted once the screen has been drawn.
constexpr char kLoadingArkPath[] = "ark/loading.ark";

constexpr int kDisplayWidth = 640;
constexpr int kDisplayHeight = 448;
constexpr int kDisplayBitDepth = 16;

// A newline follows every kLoadingDotsPerLine progress dots.
constexpr int kLoadingDotsPerLine = 64;

int g_nLoadingDots;

const char *g_pszLastFailure;

// 0x001f2638
void RecordFailMessage(const char *pszMessage) {
    g_debugStream << pszMessage;
    g_pszLastFailure = pszMessage;
}

// 0x001f2670
void HaltOnFailure() {
    Fatal(g_pszLastFailure);
}

// 0x001ef620
void ShowLoadingScreen() {
    RndAsyncLoader loader(HxStr("loading/"), HxStr("loading.rnd"), -1);
    loader.Enqueue();

    float flProgress = 0.0f;
    while (loader.Poll(&flProgress) == 0) {
        ++g_nLoadingDots;
        LogPrintf(".%s", (g_nLoadingDots & (kLoadingDotsPerLine - 1)) == 0 ? "\n" : "");
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
    InitAsync();

    g_failSink.SetReportHandler(RecordFailMessage);
    g_failSink.mAbortProc = HaltOnFailure;

    Rnd::g_manager.Init();
    g_gfxDevice.Init(kDisplayWidth, kDisplayHeight, kDisplayBitDepth);

    if (UsingArkFiles() != 0) {
        if (ArkFile::Open(kLoadingArkPath) == 0) {
            Fatal("Can't open loading.ark arkfile!\n");
        }
    }

    ShowLoadingScreen();

    if (UsingArkFiles() != 0) {
        ArkFile::Close(kLoadingArkPath);
    }

    InitArk(); // Yes, the binary discards this call's result.
    LoadIopModules();

    Application::shared()->Run(); // Yes, the binary discards this call's result.
    return 0;
}
