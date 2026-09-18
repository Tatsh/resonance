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
