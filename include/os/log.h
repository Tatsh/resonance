#pragma once

#include <stdarg.h>

#include "os/hxstr.h"

/**
 * Write a formatted message to the debug console.
 *
 * The console is `stdout`. The routine reports nothing of its own and forwards everything to the C
 * library.
 *
 * @param pszFormat A printf-style format string.
 * @ghidraAddress 0x0053dde0
 */
void LogPrintf(const char *pszFormat, ...);

/**
 * Report a recoverable problem and continue.
 *
 * Unlike Fatal(), this returns to its caller. Nothing is reported at all unless WarningsEnabled()
 * reports 1, which is the only use either the flag or this routine makes of that boot option.
 *
 * The message goes to ShowReportedMessage() rather than to the console. A warning therefore shares
 * the display path with the failure reports and not with LogPrintf().
 *
 * @param pszFormat A printf-style format string.
 * @ghidraAddress 0x0052e3e8
 */
void Warn(const char *pszFormat, ...);

/**
 * Report an unrecoverable error and stop the machine.
 *
 * The memory log is closed and reported first, then the message is formatted into a static buffer,
 * after which the routine spins forever. It never returns to its caller.
 *
 * Nothing in the image reads that buffer. A fatal error therefore stops the machine with the
 * message in memory and nothing on the console, which is what makes a debugger the only way to
 * recover the text.
 *
 * @param pszFormat A printf-style format string.
 * @ghidraAddress 0x0052e868
 */
void Fatal(const char *pszFormat, ...) __attribute__((noreturn));

/**
 * Format a message the way the failure reports do.
 *
 * The routine belongs to another translation unit and is declared here so log.cpp can call it. The
 * format is formatted into a 4096-byte stack buffer and copied into the returned string, so a
 * longer result is a stack overrun.
 *
 * An empty format string is replaced by the string the word at 0x006fbd10 addresses, which is the
 * one use of that word.
 *
 * The name is inferred. Nothing in the image attests it.
 *
 * @param format The format string.
 * @param args The arguments for it.
 * @param nUnknown Undetermined. The body never reads it, and the one caller passes 1.
 * @return The formatted message.
 * @ghidraAddress 0x005e4178
 */
HxStr FormatMessage(const HxStr &format, va_list args, int nUnknown);

/**
 * Show a message and write it to the report stream.
 *
 * The routine belongs to another translation unit and is declared here so log.cpp can call it. The
 * on-screen half is skipped unless ScreenMessagesEnabled() reports 1. The stream half always runs.
 *
 * The name is inferred. Nothing in the image attests it.
 *
 * @param text The message.
 * @param nDuration How long to show it, in the same unit ShowScreenMessage() takes.
 * @ghidraAddress 0x00466368
 */
void ShowReportedMessage(const HxStr &text, int nDuration);

/**
 * Show a message for a while, rather than writing it to the console.
 *
 * The body of this routine is a bare return in the shipped build. None of the three messages that
 * go through it therefore ever appears. The reconstruction retains the call sites because the
 * strings and the calls are both in the image. The routine sits with the C library rather than with
 * the routines above, so log.cpp declares it without defining it.
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
