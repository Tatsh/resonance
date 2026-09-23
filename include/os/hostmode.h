#pragma once

#include "os/hxstr.h"

/**
 * Media the game loads its data from.
 *
 * These names come from the start-up banner. It prints the selected mode followed by
 * " mode being used for IOP initialization.".
 */
enum HostMode {
    kHostModeHostOnly = 0, /*!< Everything is read over the host link. */
    kHostModeCdHost = 1,   /*!< The disc is preferred, with the host link as a fallback. */
    kHostModeCdOnly = 2    /*!< Retail: the disc only. */
};

/**
 * The data source selected for this run.
 *
 * @ghidraAddress 0x0050ef90
 */
HostMode GetHostMode();

/**
 * Whether data is read from ark archives rather than loose host files.
 *
 * The flag behind this accessor is a word rather than a byte. It is therefore returned as an
 * `int`.
 *
 * @return Non-zero when ark archives are in use.
 * @ghidraAddress 0x0050efa0
 */
int UsingArkFiles();

/**
 * Whether data is being read from the disc.
 *
 * The flag is one word of a block of nine boot options at 0x0070bf10 that each have an accessor of
 * this shape. The retail configurator at 0x0050f030, which InitIop() calls first, writes the block
 * in one pass and sets this word to 1 in the same instruction run that sets GetHostMode() to
 * kHostModeCdOnly and UsingArkFiles() to 1. The `.data` default is zero, which is the
 * host-development configuration. It is a separate word from the one UsingArkFiles() reads at
 * 0x0070bf14.
 *
 * Both uses agree with that reading. InitAsync() starts the worker thread only when this reports
 * the disc, because a host-link read needs no latency hiding, and ArkFile::Open() searches the
 * path for a device prefix only then, because a prefix such as `cdrom0:` exists on no host path.
 *
 * @return Non-zero when data is read from the disc.
 * @ghidraAddress 0x0050efd0
 */
int UsingCdMedia();

/**
 * Whether a MIDI conversion writes its error log.
 *
 * Another word of the same boot-option block, at 0x0070bf28. The retail configurator writes 0
 * there, and the `.data` default is also 0. LevelConverter is the only caller. It tests the word
 * before it opens the log and before each line it writes. The title is inferred.
 *
 * @return Non-zero when the error log is written.
 * @ghidraAddress 0x0050eff0
 */
int MidiErrorLogEnabled();

/**
 * Whether Warn() reports anything.
 *
 * Another word of the same boot-option block, at 0x0070bf18. Warn() is the only reader of either
 * the word or this accessor, and it reports nothing unless the answer is 1, which is what fixes
 * the meaning of the word.
 *
 * @return 1 when warnings are reported.
 * @ghidraAddress 0x0050efb0
 */
int WarningsEnabled();

/**
 * Whether a reported message also goes to the screen.
 *
 * Another word of the same boot-option block, at 0x0070bf1c. ReportMessage() is the only reader of
 * either the word or this accessor, and it skips the on-screen half unless the answer is 1.
 *
 * @return 1 when messages go to the screen.
 * @ghidraAddress 0x0050efc0
 */
int ScreenMessagesEnabled();

/**
 * Root the game composes every data path against.
 *
 * The shipped build returns the empty string, so every composed path is relative. The routine
 * shares a translation unit with GetHostMode() and UsingArkFiles(), which is what places it here
 * rather than with any one of its ten callers across six subsystems.
 *
 * @return The root, empty in the shipped build.
 * @ghidraAddress 0x0050ef30
 */
HxStr GetFreqRoot();

/**
 * Compose a data path against GetFreqRoot().
 *
 * The routine sits in the same translation unit as GetFreqRoot(). Its callers include the
 * asynchronous loader, the `save_rnd` script command, and GamePlayback, each of which opens the
 * returned path. The title is inferred.
 *
 * @param name The path below the root.
 * @return The root followed by name.
 * @ghidraAddress 0x0050d9f0
 */
HxStr MakeFreqPath(const HxStr &name);

/**
 * Whether a held modifier turns a controller button into a debug script hook.
 *
 * The boot-option word at 0x0070bf24. InputPoller::Poll() is the one reader. While it reports
 * non-zero, a button pressed with the modifier bits runs script template 0x3f2 instead of the
 * usual `'joy '` message. ConfigureRetailBoot() clears it. The name is inferred.
 *
 * @return Non-zero when the debug hooks are active.
 * @ghidraAddress 0x0050efe0
 */
int DebugKeysEnabled();

/**
 * Whether the asynchronous loader brackets each load with memory accounting.
 *
 * The boot-option word at 0x0070bf2c. RndAsyncLoader::PollAsyncLoads() calls
 * MemBeginAccounting() before reading a completed load and reports the accounting afterwards
 * while it is non-zero. ConfigureRetailBoot() clears it. The name is inferred.
 *
 * @return Non-zero when loads are accounted.
 * @ghidraAddress 0x0050f000
 */
int MemAccountingEnabled();

/**
 * Whether the start-up sequence plays the intro movie `ps2intro.pss`.
 *
 * The boot-option word at 0x0070bf30. MetSonyScreen is the one reader. ConfigureRetailBoot()
 * sets it. The name is inferred.
 *
 * @return Non-zero when the intro movie plays.
 * @ghidraAddress 0x0050f010
 */
int IntroMovieEnabled();

/**
 * Report the build version, "198" in the shipped build.
 *
 * The title screen shows it after "Version:". The unit's static initialiser builds the string, the
 * HxStr at 0x0070bf38. The name is inferred.
 *
 * @return A copy of the version string.
 * @ghidraAddress 0x0050ef60
 */
HxStr GetVersionString();

/**
 * Report whether a file can be opened for reading through the file service.
 *
 * The file is opened read-only and closed again at once. No call site survives in the shipped
 * program, and the name is inferred.
 *
 * @param pszPath The path to test.
 * @return True when the open succeeded.
 * @ghidraAddress 0x0050f0c8
 */
bool FileExists(const char *pszPath);

/**
 * Release every zone, the counterpart of InitBootConfig().
 *
 * The body is one call to ReleaseAllZoneSlots(). No call site survives in the shipped program,
 * and the name is inferred from the counterpart.
 *
 * @ghidraAddress 0x0050f0a8
 */
void TerminateBootConfig();
