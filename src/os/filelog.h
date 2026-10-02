#pragma once

#include <fstream>

/**
 * Bytes of g_szFileLogPath.
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
 * for every file it opens while the log is open. The title is inferred from that role, and it is
 * distinct from the allocation log behind `g_szMemLogPath`.
 *
 * @ghidraAddress 0x006ee280
 */
extern std::fstream g_fileLog;

/**
 * Non-zero while g_fileLog is open.
 *
 * @ghidraAddress 0x006ee320
 */
extern int g_bFileLogOpen;

/**
 * The path g_fileLog was opened on.
 *
 * FileOpen() compares every path it opens against this one and does not log the log file itself.
 *
 * @ghidraAddress 0x006ee378
 */
extern char g_szFileLogPath[kFileLogPathSize];

/**
 * Open the file log.
 *
 * The path is recorded in g_szFileLogPath and the file is opened for output. No call site exists.
 * The title is inferred.
 *
 * @param pszPath The path of the log file.
 * @ghidraAddress 0x0047ddf0
 */
void FileLogStart(const char *pszPath);

/**
 * Close the file log, if it is open.
 *
 * HxScript's `memlog_term` binding calls it at `0x00155e0c`. The title is inferred.
 *
 * @ghidraAddress 0x0047de48
 */
void FileLogStop();

/**
 * Write one line to the file log, if it is open.
 *
 * No call site exists. FileOpen() expands the same body inline. The title is inferred.
 *
 * @param pszText The line, without its line break.
 * @ghidraAddress 0x0047de88
 */
void FileLogAppend(const char *pszText);
