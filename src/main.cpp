#include <iostream>
#ifdef VIDEO_STANDARD_PAL
#include <libdma.h>
#include <libgraph.h>
#endif

#include "app/application.h"
#include "gfx/gfxdevice.h"
#include "os/async.h"
#include "os/dbg.h"
#include "os/hostmode.h"
#include "os/hxstr.h"
#include "os/iop.h"
#include "os/log.h"
#include "os/openarkobject.h"
#include "os/zone.h"
#include "rnd/asyncloader.h"
#include "rnd/manager.h"
#include "rnd/view.h"

namespace {

// The loading screen has its own archive, which is mounted before the session archives and
// unmounted once the screen has been drawn.
constexpr char kLoadingArkPath[] = "ark/loading.ark";

#ifdef VIDEO_STANDARD_PAL
constexpr int kDisplayWidth = 512;
constexpr int kDisplayHeight = 512;
#else
constexpr int kDisplayWidth = 640;
constexpr int kDisplayHeight = 448;
#endif
constexpr int kDisplayBitDepth = 16;

// A newline follows every kLoadingDotsPerLine progress dots.
constexpr int kLoadingDotsPerLine = 64;

#ifdef VIDEO_STANDARD_PAL
// The channel table main() copies before it waits for DMA to finish. It is the identity mapping of
// the ten channel numbers.
constexpr int kExitDmaChannels[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
constexpr int kExitDmaWaitCount = sizeof(kExitDmaChannels) / sizeof(kExitDmaChannels[0]);

// The sceDmaSync() mode that blocks, and the polls it makes before it gives up.
constexpr int kDmaSyncBlocking = 0;
constexpr int kExitDmaSyncTimeout = 1000;
#endif

int g_nLoadingDots = 0;

const char *g_szLastFailure;

// NTSC-U/C: 0x001f2638, PAL: 0x001f8f48
void RecordFailMessage(const char *pszMessage) {
    std::cout << pszMessage;
    g_szLastFailure = pszMessage;
}

// NTSC-U/C: 0x001f2670, PAL: 0x001f8f80
void HaltOnFailure() {
    Fatal(g_szLastFailure);
}

// NTSC-U/C: 0x001f2698, PAL: 0x001f8fa8
// ShowLoadingScreen expands this inline; the out-of-line copy has no caller.
inline void PrintLoadingDot() {
    ++g_nLoadingDots;
    LogPrintf(".%s", (g_nLoadingDots & (kLoadingDotsPerLine - 1)) == 0 ? "\n" : "");
}

#ifdef VIDEO_STANDARD_PAL
// The file-name suffix of the console language's loading screen, empty for English.
inline const char *GetLoadingScreenSuffix() {
    switch (GetLanguage()) {
    case SCE_GERMAN_LANGUAGE:
        return "_ger";
    case SCE_FRENCH_LANGUAGE:
        return "_fre";
    case SCE_ITALIAN_LANGUAGE:
        return "_ita";
    case SCE_SPANISH_LANGUAGE:
        return "_spa";
    default:
        return "";
    }
}

// PAL: 0x001f8ff8
// main() expands WaitForExitDma() inline, and the out-of-line copy has no caller.
inline void WaitForExitDma() {
    int channels[kExitDmaWaitCount];
    for (int i = 0; i < kExitDmaWaitCount; ++i) {
        channels[i] = kExitDmaChannels[i];
    }
    for (int i = 0; i < kExitDmaWaitCount; ++i) {
        // Yes, the binary waits on the first channel of the table every time.
        sceDmaSync(sceDmaGetChan(channels[0]), kDmaSyncBlocking, kExitDmaSyncTimeout);
    }
}
#endif

// NTSC-U/C: 0x001ef620, PAL: 0x001f59d8
void ShowLoadingScreen() {
#ifdef VIDEO_STANDARD_PAL
    const HxStr baseName("loading");
    const HxStr extension(".rnd");
    const HxStr suffix(GetLoadingScreenSuffix());
    const HxStr fileName = baseName + suffix + extension;
    RndAsyncLoader loader(HxStr("loading/"), fileName, -1);
#else
    RndAsyncLoader loader(HxStr("loading/"), HxStr("loading.rnd"), -1);
#endif
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

// NTSC-U/C: 0x001ef870, PAL: 0x001f5fb0
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
        if (OpenArkObject::Open(kLoadingArkPath) == 0) {
            Fatal("Can't open loading.ark arkfile!\n");
        }
    }

    ShowLoadingScreen();

    if (UsingArkFiles() != 0) {
        OpenArkObject::Close(kLoadingArkPath);
    }

    InitArk(); // Yes, the binary discards this call's result.
    LoadIopModules();

    Application::shared()->Run(); // Yes, the binary discards this call's result.
#ifdef VIDEO_STANDARD_PAL
    Application::shared()->ExitInstance(); // Yes, the binary discards this call's result.
    ShutdownIop();
    CloseArk(); // Yes, the binary discards this call's result.
    WaitForExitDma();
    sceGsSyncVCallback(nullptr);
#endif
    return 0;
}
