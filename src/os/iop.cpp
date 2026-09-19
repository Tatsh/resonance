#include "os/iop.h"

#include <algorithm>
#include <ctype.h>
#include <iostream.h>
#include <libcdvd.h>
#include <libmc.h>
#include <libmtap.h>
#include <sifdev.h>
#include <sifrpc.h>
#include <stdlib.h>

#include "os/async.h"
#include "os/hostmode.h"
#include "os/hxstr.h"

namespace {

constexpr char kDiscImagePath[] = "cdrom0:\\IOP\\IOPRP23.IMG;1";
constexpr char kHostImagePath[] = "host0:iop/ioprp23.img";

constexpr char kDiscPathPrefix[] = "cdrom0:\\IOP\\";
constexpr char kDiscPathSuffix[] = ".IRX;1";
constexpr char kHostPathPrefix[] = "host0:iop/";
constexpr char kHostPathSuffix[] = ".irx";

constexpr char kLoadFailedFormat[] = "Couldn't load IOP module %s";
constexpr char kNoSourceMessage[] = "LoadModuleFromAnywhere failed";

constexpr char kCdHostModeName[] = " CD_HOST";
constexpr char kCdOnlyModeName[] = " CD_ONLY";
constexpr char kHostOnlyModeName[] = " HOST_ONLY";
constexpr char kModeBannerTail[] = " mode being used for IOP initialization.\n";

// The exit status every failure in this file reports.
constexpr int kLoadFailureStatus = 1;

// Read off the walk at 0x004de6d8, which ends 0x78 bytes past the base of a table of 12-byte
// records.
constexpr int kIopModuleCount = 10;

// 0x00702660. Labelled g_abIopModules in the program, because the naming policy there has no prefix
// for a typed aggregate. The table sits in .data rather than in .rodata, so the original declared
// it without const.
IopModule g_iopModules[kIopModuleCount] = {{"sio2man", 0, nullptr},
                                           {"mcman", 0, nullptr},
                                           {"mcserv", 0, nullptr},
                                           {"usbd", 0, nullptr},
                                           {"padman", 0, nullptr},
                                           {"msifrpc", 0, nullptr},
                                           {"libsd", 0, nullptr},
                                           {"mtapman", 0, nullptr},
                                           {"sdrdrv", 0, nullptr},
                                           {"ezmidi", 0, nullptr}};

// 0x004dfc40. An unreferenced out-of-line copy sits at that address while the only call site is
// inlined, which is what establishes an inline function rather than a block the caller open-codes.
// The same applies to every helper below.
inline void LoadModuleFromHost(const char *pszPath, int nArgLength, const char *pArgs) {
    if (sceSifLoadModule(pszPath, nArgLength, pArgs) < 0) {
        Error(kLoadFailedFormat, pszPath);
        exit(kLoadFailureStatus);
    }
}

// 0x004dfc80
inline void LoadModuleFromCd(const char *pszPath, int nArgLength, const char *pArgs) {
    if (sceSifLoadModule(pszPath, nArgLength, pArgs) < 0) {
        Error(kLoadFailedFormat, pszPath);
        exit(kLoadFailureStatus);
    }

    sceCdSync(SCECdBlock);
}

// 0x004dfd80. The host mode is read again here rather than passed in, so InitIop() and this helper
// each call GetHostMode() once.
inline void RebootIopWithImage() {
    sceSifInitRpc(0);
    sceCdInit(SCECdINIT);
    sceCdMmode(SCECdMmodeCd);

    if (GetHostMode() == kHostModeCdOnly) {
        while (sceSifRebootIop(kDiscImagePath) == 0) {
        }
    } else {
        while (sceSifRebootIop(kHostImagePath) == 0) {
        }
    }

    while (sceSifSyncIop() == 0) {
    }

    sceFsReset();
    sceSifInitRpc(0);
    sceCdInit(SCECdINIT);
    sceCdMmode(SCECdMmodeCd);
}

// 0x004dfcc8. Named WalkIopModuleTable in the program, whose naming guard refuses this spelling as
// a token superset of LoadIopModules.
inline void LoadAllIopModules(unsigned nSources) {
    AsyncCheck(1);

    const IopModule *const pEnd = &g_iopModules[kIopModuleCount];
    for (const IopModule *pModule = g_iopModules; pModule < pEnd; ++pModule) {
        LoadIopModule(pModule, nSources);
    }
}

// 0x004dfd28. The console exposes two controller ports, and a third is opened regardless.
inline void InitMultitapPorts() {
    sceMtapInit();
    sceMtapPortOpen(0);
    sceMtapPortOpen(1);
    sceMtapPortOpen(2);
}

// 0x004dfd60
inline void InitMemoryCardLibrary() {
    sceMcInit(); // Yes, the binary discards this call's result.
}

} // namespace

void InitIop() {
    ConfigureRetailBoot();
    RebootIopWithImage();
    InitDebugConsole();
    InitBootConfig();
}

void LoadIopModule(const IopModule *pModule, unsigned nSources) {
    if ((nSources & kIopModuleSourceDisc) != 0) {
        HxStr name(pModule->mName);
        std::transform(name.mStr, name.mStr + name.mLen, name.mStr, toupper);

        HxStr path = HxStr(kDiscPathPrefix) + name + kDiscPathSuffix;
        LoadModuleFromCd(path.mStr != nullptr ? path.mStr : g_szEmptyString,
                         pModule->mArgLength,
                         pModule->mpArgs);
    } else if ((nSources & kIopModuleSourceHost) != 0) {
        HxStr path = HxStr(kHostPathPrefix) + pModule->mName + kHostPathSuffix;
        LoadModuleFromHost(path.mStr != nullptr ? path.mStr : g_szEmptyString,
                           pModule->mArgLength,
                           pModule->mpArgs);
    } else {
        Error(kNoSourceMessage);
        exit(kLoadFailureStatus);
    }
}

void LoadIopModules() {
    const HostMode mode = GetHostMode();
    WaitVsync();

    switch (mode) {
    case kHostModeCdHost:
        cout << kCdHostModeName;
        break;
    case kHostModeCdOnly:
        cout << kCdOnlyModeName;
        break;
    case kHostModeHostOnly:
        cout << kHostOnlyModeName;
        break;
    }
    cout << kModeBannerTail;

    sceSifInitRpc(0);
    sceSifInitIopHeap();

    unsigned nSources = kIopModuleSourceHost;
    if (mode == kHostModeCdHost) {
        nSources = kIopModuleSourceDisc | kIopModuleSourceHost;
    } else if (mode == kHostModeCdOnly) {
        nSources = kIopModuleSourceDisc;
    }

    LoadAllIopModules(nSources);

    RegisterHardEffectCommands();
    InitMultitapPorts();
    InitMemoryCardLibrary();
}
