#pragma once

/** Callback that a long operation invokes while it blocks the frame loop. */
typedef void (*LongOperationProc)();

/**
 * Install the callback that a long operation pumps its timers through.
 *
 * A null callback removes the previous one.
 *
 * @param pfnPoll The callback to install.
 * @ghidraAddress NTSC-U/C: 0x00520428
 * @ghidraAddress PAL: 0x00560980
 */
void SetLongOperationPollProc(LongOperationProc pfnPoll);

/**
 * Call the installed poll callback, if there is one.
 *
 * GfxDevice::FlushGifPacket() calls it after every packet it submits.
 *
 * @ghidraAddress NTSC-U/C: 0x00520438
 * @ghidraAddress PAL: 0x00560990
 */
void RunLongOperationPollProc();

/**
 * Install the callback that a long operation refreshes the display through.
 *
 * A null callback removes the previous one.
 *
 * @param pfnDraw The callback to install.
 * @ghidraAddress NTSC-U/C: 0x00520460
 * @ghidraAddress PAL: 0x005609b8
 */
void SetLongOperationDrawProc(LongOperationProc pfnDraw);

/**
 * Call the installed draw callback, if there is one.
 *
 * @ghidraAddress NTSC-U/C: 0x00520470
 * @ghidraAddress PAL: 0x005609c8
 */
void RunLongOperationDrawProc();

/**
 * The installed poll callback, or null.
 *
 * @ghidraAddress NTSC-U/C: 0x00719858
 * @ghidraAddress PAL: 0x0075d758
 */
extern LongOperationProc g_pfnLongOperationPollProc;

/**
 * The installed draw callback, or null.
 *
 * Rnd::Manager::Read() tests and calls it directly rather than through RunLongOperationDrawProc().
 *
 * @ghidraAddress NTSC-U/C: 0x0071985c
 * @ghidraAddress PAL: 0x0075d75c
 */
extern LongOperationProc g_pfnLongOperationDrawProc;
