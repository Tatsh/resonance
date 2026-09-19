#pragma once

/**
 * Write a formatted message to the debug console.
 *
 * @param pszFormat A printf-style format string.
 * @ghidraAddress 0x0053dde0
 */
void LogPrintf(const char *pszFormat, ...);

/**
 * Report a recoverable problem and continue.
 *
 * Unlike Fatal(), this returns to its caller.
 *
 * @param pszFormat A printf-style format string.
 * @ghidraAddress 0x0052e3e8
 */
void Warn(const char *pszFormat, ...);

/**
 * Report an unrecoverable error and stop the machine.
 *
 * The message is formatted and printed, after which the routine spins forever; it never returns
 * to its caller.
 *
 * @param pszFormat A printf-style format string.
 * @ghidraAddress 0x0052e868
 */
void Fatal(const char *pszFormat, ...) __attribute__((noreturn));

/**
 * Show a message for a while, rather than writing it to the console.
 *
 * The body of this routine is a bare return in the shipped build. None of the three messages that
 * go through it therefore ever appears. The reconstruction retains the call sites because the
 * strings and the calls are both in the image.
 *
 * The text is already formatted at every call site, so the routine is not variadic. Both the name
 * and the second argument's unit are inferred: the three callers pass a duration of 300, 600, and a
 * forwarded parameter, and nothing in the image titles either the routine or the value.
 *
 * @param pszText The message.
 * @param nDuration How long to show it.
 * @ghidraAddress 0x005e5ed0
 */
void ShowScreenMessage(const char *pszText, int nDuration);
