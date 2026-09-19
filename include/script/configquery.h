#pragma once

#include "os/hxstr.h"

/**
 * Read one configuration value through the embedded interpreter.
 *
 * The routine builds an integer object from the code, wraps it in an event tuple, evaluates that
 * tuple against the script host under opcode 0x102, and coerces the result to an integer. It
 * reports 1 when the value is non-zero and 0 otherwise, so a code that selects a flag is read
 * directly and a code that selects a number is not.
 *
 * Every code is numeric and no table in the image maps a code to a name. The codes recovered so
 * far are 0x39a for the async loader state, 0x3a1 and 0x3a4 for display mode, and 0x398 for one
 * of the mixer flags.
 *
 * The body is not reconstructed.
 *
 * @param nEventCode The configuration code.
 * @return 1 when the value is non-zero.
 * @ghidraAddress 0x005093e0
 */
int QueryConfigFlag(int nEventCode);

/**
 * Read one configuration value as an integer through the embedded interpreter.
 *
 * The same evaluation as QueryConfigFlag() with the coercion returning the value rather than a
 * flag. The trailing arguments are substituted into the property lookup.
 *
 * The body is not reconstructed.
 *
 * @param nEventCode The configuration code.
 * @return The value.
 * @ghidraAddress 0x00509110
 */
int QueryConfigValue(int nEventCode, ...);

/**
 * Read one configuration value as a string through the embedded interpreter.
 *
 * The same evaluation as QueryConfigFlag() with the result coerced to a string, returned through
 * the caller's HxStr rather than in a register. The trailing arguments are substituted into the
 * property lookup. Ps2HardSynth's three bank loaders each read a pair of bank paths this way,
 * passing GetHostMode() as the one substituted argument.
 *
 * The body is not reconstructed.
 *
 * @param pResult The string the value is written to.
 * @param nEventCode The configuration code.
 * @return pResult.
 * @ghidraAddress 0x005096d0
 */
HxStr *QueryConfigString(HxStr *pResult, int nEventCode, ...);

/**
 * Fill a vector from one configuration value through the embedded interpreter.
 *
 * Mixer's constructor reads its per-track table this way under code 0x39f.
 *
 * The body is not reconstructed.
 *
 * @param pResult The vector the values are written to, as its three raw words.
 * @param nEventCode The configuration code.
 * @ghidraAddress 0x0050a1a0
 */
void QueryConfigVector(void *pResult, int nEventCode);
