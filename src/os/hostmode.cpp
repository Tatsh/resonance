#include "os/hostmode.h"

#include <libscf.h>
#include <sifdev.h>

#include "os/iop.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"

namespace {

// The boot options are nine consecutive words from 0x0070bf10, each with an accessor of the same
// three-instruction shape, followed by the version string. Both writers of the block are in this
// translation unit, ConfigureRetailBoot() and ForceCdOnlyBoot().

// The value of the file-service open flag that opens for reading only.
constexpr int kOpenReadOnly = 1;

// NTSC-U/C: 0x0070bf10, PAL: 0x0074faa0
int g_nHostMode = 0;

// NTSC-U/C: 0x0070bf14, PAL: 0x0074faa4
int g_nUsingArkFiles = 0;

// NTSC-U/C: 0x0070bf18, PAL: 0x0074faa8
int g_nWarningsEnabled = 1;

// No routine writes this word. ConfigureRetailBoot() omits it from the block.
// NTSC-U/C: 0x0070bf1c, PAL: 0x0074faac
int g_nScreenMessagesEnabled = 1;

// NTSC-U/C: 0x0070bf20, PAL: 0x0074fab0
int g_nUsingCdMedia = 0;

// NTSC-U/C: 0x0070bf24, PAL: 0x0074fab4
int g_nDebugKeysEnabled = 0;

// NTSC-U/C: 0x0070bf28, PAL: 0x0074fab8
int g_nMidiErrorLogEnabled = 0;

// NTSC-U/C: 0x0070bf2c, PAL: 0x0074fabc
int g_nMemAccountingEnabled = 0;

// NTSC-U/C: 0x0070bf30, PAL: 0x0074fac0
int g_nIntroMovieEnabled = 1;

// NTSC-U/C: 0x0070bf38, PAL: 0x0074fac8
#ifdef VIDEO_STANDARD_PAL
HxStr g_versionString("197");
#else
HxStr g_versionString("198");
#endif

#ifdef VIDEO_STANDARD_PAL
// The language code GetLanguage() reports. The name is inferred.
// PAL: 0x0074fad0
int g_nLanguage = SCE_FRENCH_LANGUAGE;
#endif

// Report that no configuration file was found, force the disc configuration, and open the memory
// report. InitBootConfig() is the one caller.
// NTSC-U/C: 0x0050dad8, PAL: 0x0054cfd0
void ForceCdOnlyBoot() {
    LogPrintf(" Running from CD only, since we couldn't find the config file\n");
    g_nHostMode = kHostModeCdOnly;
    LogPrintf(" Running from CD ONLY, forcing arkfiles ON and async ON\n");
    g_nUsingArkFiles = 1;
    g_nUsingCdMedia = 1;
    MemOpenLog(nullptr);
}

} // namespace

// NTSC-U/C: 0x0050ef90, PAL: 0x0054e508
HostMode GetHostMode() {
    return static_cast<HostMode>(g_nHostMode);
}

// NTSC-U/C: 0x0050efa0, PAL: 0x0054e518
int UsingArkFiles() {
    return g_nUsingArkFiles;
}

// NTSC-U/C: 0x0050efb0, PAL: 0x0054e528
int WarningsEnabled() {
    return g_nWarningsEnabled;
}

// NTSC-U/C: 0x0050efc0, PAL: 0x0054e538
int ScreenMessagesEnabled() {
    return g_nScreenMessagesEnabled;
}

// NTSC-U/C: 0x0050efd0, PAL: 0x0054e548
int UsingCdMedia() {
    return g_nUsingCdMedia;
}

// NTSC-U/C: 0x0050ef30, PAL: 0x0054e4a8
HxStr GetFreqRoot() {
    return HxStr("");
}

// NTSC-U/C: 0x0050d9f0, PAL: 0x0054cea8
HxStr MakeFreqPath(const HxStr &name) {
    HxStr path(GetFreqRoot());
    path += name;
    return path;
}

// NTSC-U/C: 0x0050efe0, PAL: 0x0054e558
int DebugKeysEnabled() {
    return g_nDebugKeysEnabled;
}

// NTSC-U/C: 0x0050eff0, PAL: 0x0054e568
int MidiErrorLogEnabled() {
    return g_nMidiErrorLogEnabled;
}

// NTSC-U/C: 0x0050f000, PAL: 0x0054e578
int MemAccountingEnabled() {
    return g_nMemAccountingEnabled;
}

// NTSC-U/C: 0x0050f010, PAL: 0x0054e588
int IntroMovieEnabled() {
    return g_nIntroMovieEnabled;
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x0054e5a8
int GetLanguage() {
    return g_nLanguage;
}

// PAL: 0x0054e5b8
void SetLanguage(int nLanguage) {
    g_nLanguage = nLanguage;
}
#endif

// NTSC-U/C: 0x0050ef60, PAL: 0x0054e4d8
HxStr GetVersionString() {
    return g_versionString;
}

// NTSC-U/C: 0x0050f030, PAL: 0x0054e5c8
void ConfigureRetailBoot() {
    g_nHostMode = kHostModeCdOnly;
    g_nIntroMovieEnabled = 1;
    g_nUsingArkFiles = 1;
    g_nWarningsEnabled = 0;
    g_nUsingCdMedia = 1;
    g_nDebugKeysEnabled = 0;
    g_nMidiErrorLogEnabled = 0;
    g_nMemAccountingEnabled = 0;
}

// NTSC-U/C: 0x0050f080, PAL: 0x0054e618
void InitBootConfig() {
    ForceCdOnlyBoot();
    InitializeZoneList();
#ifdef VIDEO_STANDARD_PAL
    SetLanguage(sceScfGetLanguage());
#endif
}

// NTSC-U/C: 0x0050f0a8, PAL: 0x0054e650
void TerminateBootConfig() {
    ReleaseAllZoneSlots();
}

// NTSC-U/C: 0x0050f0c8, PAL: 0x0054e670
bool FileExists(const char *pszPath) {
    const int nDescriptor = sceOpen(pszPath, kOpenReadOnly);
    if (nDescriptor < 0) {
        return false;
    }
    sceClose(nDescriptor);
    return true;
}
