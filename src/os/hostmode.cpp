#include "os/hostmode.h"

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

// 0x0070bf10
int g_nHostMode;

// 0x0070bf14
int g_nUsingArkFiles;

// 0x0070bf18
int g_nWarningsEnabled;

// 0x0070bf1c
int g_nScreenMessagesEnabled;

// 0x0070bf20
int g_nUsingCdMedia;

// 0x0070bf24
int g_nDebugKeysEnabled;

// 0x0070bf28
int g_nMidiErrorLogEnabled;

// 0x0070bf2c
int g_nMemAccountingEnabled;

// 0x0070bf30
int g_nIntroMovieEnabled;

// 0x0070bf38
HxStr g_versionString("198");

// Report that no configuration file was found, force the disc configuration, and open the memory
// report. InitBootConfig() is the one caller.
// 0x0050dad8
void ForceCdOnlyBoot() {
    LogPrintf(" Running from CD only, since we couldn't find the config file\n");
    g_nHostMode = kHostModeCdOnly;
    LogPrintf(" Running from CD ONLY, forcing arkfiles ON and async ON\n");
    g_nUsingArkFiles = 1;
    g_nUsingCdMedia = 1;
    MemOpenLog(nullptr);
}

} // namespace

HostMode GetHostMode() {
    return static_cast<HostMode>(g_nHostMode);
}

int UsingArkFiles() {
    return g_nUsingArkFiles;
}

int WarningsEnabled() {
    return g_nWarningsEnabled;
}

int ScreenMessagesEnabled() {
    return g_nScreenMessagesEnabled;
}

int UsingCdMedia() {
    return g_nUsingCdMedia;
}

HxStr GetFreqRoot() {
    return HxStr("");
}

// 0x0050d9f0
HxStr MakeFreqPath(const HxStr &name) {
    HxStr path(GetFreqRoot());
    path += name;
    return path;
}

// 0x0050efe0
int DebugKeysEnabled() {
    return g_nDebugKeysEnabled;
}

// 0x0050eff0
int MidiErrorLogEnabled() {
    return g_nMidiErrorLogEnabled;
}

// 0x0050f000
int MemAccountingEnabled() {
    return g_nMemAccountingEnabled;
}

// 0x0050f010
int IntroMovieEnabled() {
    return g_nIntroMovieEnabled;
}

// 0x0050ef60
HxStr GetVersionString() {
    return g_versionString;
}

// 0x0050f030
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

// 0x0050f080
void InitBootConfig() {
    ForceCdOnlyBoot();
    InitializeZoneList();
}

// 0x0050f0a8
void TerminateBootConfig() {
    ReleaseAllZoneSlots();
}

// 0x0050f0c8
bool FileExists(const char *pszPath) {
    const int nDescriptor = sceOpen(pszPath, kOpenReadOnly);
    if (nDescriptor < 0) {
        return false;
    }
    sceClose(nDescriptor);
    return true;
}
