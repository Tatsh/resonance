#pragma once

#include <vector>

#include "os/hxstr.h"

/**
 * Read one configuration value as a flag through the embedded interpreter.
 *
 * Every query here works the same way. The code selects a registered script call template through
 * GetScriptTemplate(), the trailing arguments are formatted into it by FormatMessage(), and the
 * text is evaluated as a Python expression through EvalScriptExpression(). A result the conversion
 * rejects raises a Py::Exception that the query catches, reports through Fatal() with the
 * expression text, and clears with PyErr_Clear().
 *
 * This query converts the result to an integer and reports 1 when it is non-zero and 0 otherwise.
 * The codes recovered so far are 0x39a for the async loader state, 0x3a1 and 0x3a4 for display
 * mode, and 0x398 for one of the mixer flags.
 *
 * @param nEventCode The template identifier.
 * @return 1 when the value is non-zero, and 0 otherwise or after a failed conversion.
 * @ghidraAddress 0x005093e0
 */
int QueryConfigFlag(int nEventCode, ...);

/**
 * Read one configuration value as an integer through the embedded interpreter.
 *
 * The same evaluation as QueryConfigFlag(), returning the integer itself.
 *
 * @param nEventCode The template identifier.
 * @return The value, or 0 after a failed conversion.
 * @ghidraAddress 0x00509110
 */
int QueryConfigValue(int nEventCode, ...);

/**
 * Read one configuration value as a string through the embedded interpreter.
 *
 * The same evaluation as QueryConfigFlag(), converting the result through Py::String. A failed
 * conversion yields an empty string. Ps2HardSynth's three bank loaders each read a pair of bank
 * paths this way, passing GetHostMode() as the one substituted argument.
 *
 * @param nEventCode The template identifier.
 * @return The value.
 * @ghidraAddress 0x005096d0
 */
HxStr QueryConfigString(int nEventCode, ...);

/**
 * Fill a vector of integers from one configuration value through the embedded interpreter.
 *
 * The result is taken as a Py::Sequence, the vector is cleared, and each element is converted
 * through Py::Int and appended. Mixer's constructor reads its per-track table this way under code
 * 0x39f, and GameEnableMgr passes a one-based track number as the substituted argument.
 *
 * @param pResult The vector the values are written to.
 * @param nEventCode The template identifier.
 * @ghidraAddress 0x0050a1a0
 */
void QueryConfigVector(std::vector<int> *pResult, int nEventCode, ...);

/**
 * Fill a vector of strings from one configuration value through the embedded interpreter.
 *
 * The same evaluation as QueryConfigVector(), converting each element through Py::String. The
 * front end's song lists read their names this way under codes 0x276 and 0x27a. The title is
 * inferred.
 *
 * @param pResult The vector the values are written to.
 * @param nEventCode The template identifier.
 * @ghidraAddress 0x00509b78
 */
void QueryConfigStrings(std::vector<HxStr> *pResult, int nEventCode, ...);
