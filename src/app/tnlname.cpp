#include "app/tnlname.h"

#include "os/formatstring.h"

int g_nAppTunnelNameCounter;

HxStr NextAppTunnelName() {
    return HxStr(FormatString("<apptnl%04d>", ++g_nAppTunnelNameCounter));
}
