#pragma once

#include "os/hxstr.h"

/**
 * Prefix every game data path is built on.
 *
 * The routine returns a copy of one fixed string, which is empty in the shipped build, so every
 * composed path is relative. PyShell's constructor builds the two `sys.path` entries on it and
 * RunMasterInitScript() builds the script path on it.
 *
 * The declaration sits here because the script layer needs it, and the placement is provisional.
 * The emitting translation unit is the boot-configuration unit that also holds GetHostMode() and
 * UsingArkFiles(), and ten callers across six subsystems read it, so it belongs in
 * `os/hostmode.h`. The `hx` module exports it to Python as `get_freq_root()`, which the shipped
 * `global/grvscript.py` calls.
 *
 * @return The prefix.
 * @ghidraAddress 0x0050ef30
 */
HxStr GetFreqRoot();
