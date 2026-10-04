#include "app/tnlname.h"

#include "os/formatstring.h"

// NTSC-U/C: 0x006e42ac, PAL: 0x00727bcc
int g_nAppTunnelNameCounter = 1;

HxStr NextAppTunnelName() {
    return HxStr(Rnd::MakeString("<apptnl%04d>", ++g_nAppTunnelNameCounter));
}
