#include "app/tnlname.h"

#include "os/formatstring.h"

// NTSC-U/C: 0x006e42ac, PAL: 0x00727bcc
int g_nAppTunnelNameCounter = 1;

// NTSC-U/C: 0x00454650, PAL: 0x00491b80
HxStr NextAppTunnelName() {
    return HxStr(FormatString("<apptnl%04d>", ++g_nAppTunnelNameCounter));
}
