#pragma once

#include <stdarg.h>

#ifdef __cplusplus
#include "os/hxstr.h"

extern "C" {
#endif

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

#ifdef __cplusplus
}

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
 * The declaration the game compiled against did not mark the routine as never returning, so its
 * callers carry code after the call (Heap::Alloc() at `0x00551780` continues into its timing
 * tail). This declaration omits the attribute for the same reason.
 *
 * @param pszFormat A printf-style format string.
 * @ghidraAddress 0x0052e868
 */
void Fatal(const char *pszFormat, ...);

/**
 * Report a problem and return to the caller.
 *
 * The message is formatted through FormatMessage() and handed to ShowAlertMessage(), with no boot
 * option able to suppress it. Unlike Fatal() it returns, and every call site in iop.cpp calls
 * exit(1) immediately afterwards.
 *
 * The name is inferred. Nothing in the image attests it.
 *
 * @param pszFormat A printf-style format string.
 * @ghidraAddress 0x0052e960
 */
void Error(const char *pszFormat, ...);

/**
 * Report a failed assertion on screen and exit.
 *
 * The report is built in a `strstream` as "Assertion failed ", the file, ":", the line, ": ", and
 * the message, then shown through ShowScreenMessage() for 600 units before `exit(0)`. No call
 * site survives in the shipped program, and the name is inferred from the text.
 *
 * @param pszMessage The message.
 * @param pszFile The reporting file.
 * @param nLine The reporting line.
 * @ghidraAddress 0x0052e510
 */
void ReportAssertion(const char *pszMessage, const char *pszFile, int nLine)
    __attribute__((noreturn));

/**
 * Show a registered script template, formatted with the arguments, as an alert.
 *
 * The template text is fetched with GetScriptTemplate(), formatted through FormatMessage(), and
 * passed to ShowAlertMessage(). No call site survives in the shipped program, and the name is
 * inferred.
 *
 * @param nTemplate The template identifier.
 * @ghidraAddress 0x0052ea68
 */
void AlertScriptTemplate(int nTemplate, ...);

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
 * Format a message, passing zero as the word FormatMessage() does not read.
 *
 * QueryConfigVector() and PythonEvt::QueryOption() format their reports through it. The title is
 * inferred.
 *
 * @param format The format string.
 * @param args The arguments for it.
 * @return The formatted message.
 * @ghidraAddress 0x005e4148
 */
HxStr FormatMessage(const HxStr &format, va_list args);

/**
 * Format a message from a C string and a variable argument list.
 *
 * The format is copied into a temporary HxStr before FormatMessage() runs. The shipped program
 * does not call it, and the title is inferred.
 *
 * @param pszFormat A printf-style format string.
 * @return The formatted message.
 * @ghidraAddress 0x005e2c80
 */
HxStr FormatHxStr(const char *pszFormat, ...);

/**
 * Format a registered script template with a variable argument list.
 *
 * The template text is fetched with GetScriptTemplate() and formatted through FormatMessage(). The
 * shipped program does not call it, and the title is inferred.
 *
 * @param nTemplate The template identifier.
 * @return The formatted message.
 * @ghidraAddress 0x005e2d38
 */
HxStr FormatScriptTemplate(int nTemplate, ...);

/**
 * Show a message and write it to the report stream.
 *
 * The on-screen half is skipped unless ScreenMessagesEnabled() reports 1. The stream half, `cout`
 * followed by `endl`, always runs.
 *
 * The name is inferred. Nothing in the image attests it.
 *
 * @param text The message.
 * @param nDuration How long to show it, in the same unit ShowScreenMessage() takes.
 * @ghidraAddress 0x00466368
 */
void ShowReportedMessage(const HxStr &text, int nDuration);

/**
 * Show a message and write it to `cout` prefixed `Alert! `.
 *
 * Unlike ShowReportedMessage(), the on-screen half always runs, for a fixed duration of 50. The
 * variadic log routines at `0x0052e960` and `0x0052ea68` are the two callers.
 *
 * The name is inferred from the prefix.
 *
 * @param text The message.
 * @ghidraAddress 0x004663d8
 */
void ShowAlertMessage(const HxStr &text);
#endif

/**
 * Show a message for a while, rather than writing it to the console.
 *
 * The body of this routine is a bare return in the shipped build. None of the three messages that
 * go through it therefore ever appears. The reconstruction retains the call sites because the
 * strings and the calls are both in the image. The routine sits in the debug console unit, between
 * InitDebugGs() and OpenDebugConsole(), and devconsole.cpp defines it.
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
