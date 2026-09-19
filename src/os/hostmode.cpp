#include "os/hostmode.h"

namespace {

// The boot options are nine consecutive words from 0x0070bf10, each with an accessor of the same
// three-instruction shape, followed by an HxStr at 0x0070bf38 with a copy accessor at 0x0050ef60.
// The three below are the ones this header declares. Both writers of the block are in this
// translation unit, the retail configurator at 0x0050f030 and the routine at 0x0050dad8, and
// neither is reconstructed yet.

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
