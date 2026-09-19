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
