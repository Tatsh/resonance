#pragma once

#include "os/hxstr.h"

/**
 * Produce the next unique object name of the form `<apptnl%04d>`.
 *
 * Advances g_nAppTunnelNameCounter first, and the first name is therefore `<apptnl0001>`.
 * TnlPanelFX's constructor inlines the body. The title is inferred from the format string and
 * from NextHudName(), a routine of the same shape.
 *
 * @return The name.
 * @ghidraAddress 0x00454650
 */
HxStr NextAppTunnelName();

/**
 * Count of names NextAppTunnelName() has produced.
 *
 * @ghidraAddress 0x006e42ac
 */
extern int g_nAppTunnelNameCounter;
