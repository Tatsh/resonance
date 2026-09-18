#pragma once

/** Callback that a long operation invokes while it blocks the frame loop. */
typedef void (*LongOperationProc)();

/**
 * Install the callback that a long operation pumps its timers through.
 *
 * A null callback removes the previous one.
 *
 * @param pfnPoll The callback to install.
 * @ghidraAddress 0x00520428
 */
void SetLongOperationPollProc(LongOperationProc pfnPoll);

/**
 * Install the callback that a long operation refreshes the display through.
 *
 * A null callback removes the previous one.
 *
 * @param pfnDraw The callback to install.
 * @ghidraAddress 0x00520460
 */
void SetLongOperationDrawProc(LongOperationProc pfnDraw);
