#pragma once

#include <fstream>

/**
 * Bytes of logfilename.
 *
 * The next initialised data begins 0x40 bytes after the buffer, which bounds it. No routine in the
 * image tests the length of a path it copies in.
 */
constexpr int kFileLogPathSize = 0x40;

/**
 * The log of the files the file layer opens.
 *
 * The file layer's translation unit constructs it in its static initialiser at `0x0047dc18`. The
 * ios virtual base sits at `+0x70`. FileOpen() at `0x0047c9c0` writes a line to the output side
 * for every file it opens while the log is open. It is distinct from the allocation log behind
 * `g_szMemLogPath`.
 *
 * @ghidraAddress NTSC-U/C: 0x006ee280
 * @ghidraAddress PAL: 0x00731ca0
 */
extern std::fstream gFileIOLog;

/**
 * Non-zero while gFileIOLog is open.
 *
 * @ghidraAddress NTSC-U/C: 0x006ee320
 * @ghidraAddress PAL: 0x00731d40
 */
extern int bFileLogging;

/**
 * The path gFileIOLog was opened on.
 *
 * FileOpen() compares every path it opens against this one and does not log the log file itself.
 *
 * @ghidraAddress NTSC-U/C: 0x006ee378
 * @ghidraAddress PAL: 0x00731d98
 */
extern char logfilename[kFileLogPathSize];

/**
 * Open the file log.
 *
 * The path is recorded in logfilename and the file is opened for output. No call site exists.
 *
 * @param pszPath The path of the log file.
 * @ghidraAddress NTSC-U/C: 0x0047ddf0
 * @ghidraAddress PAL: 0x004bbac8
 */
void InitFileIOLog(char *pszPath);

/**
 * Close the file log, if it is open.
 *
 * HxScript's `memlog_term` binding calls it at `0x00155e0c`.
 *
 * @ghidraAddress NTSC-U/C: 0x0047de48
 * @ghidraAddress PAL: 0x004bbb20
 */
void CloseFileIOLog();

/**
 * Write one line to the file log, if it is open.
 *
 * No call site exists. FileOpen() expands the same body inline.
 *
 * @param pszText The line, without its line break.
 * @ghidraAddress NTSC-U/C: 0x0047de88
 * @ghidraAddress PAL: 0x004bbb60
 */
void PrintToFileIOLog(const char *pszText);
