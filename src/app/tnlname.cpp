#include "app/tnlname.h"

#include "os/formatstring.h"

// 0x006e42ac
int g_nAppTunnelNameCounter;

// 0x00454650
HxStr NextAppTunnelName() {
    return HxStr(FormatString("<apptnl%04d>", ++g_nAppTunnelNameCounter));
}
