#pragma once

#include <libscf.h>

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
 * @ghidraAddress NTSC-U/C: 0x0050ef90
 * @ghidraAddress PAL: 0x0054e508
 */
HostMode GetHostMode();

/**
 * Whether data is read from ark archives rather than loose host files.
 *
 * The flag behind this accessor is a word rather than a byte. It is therefore returned as an
 * `int`.
 *
 * @return Non-zero when ark archives are in use.
 * @ghidraAddress NTSC-U/C: 0x0050efa0
 * @ghidraAddress PAL: 0x0054e518
 */
int UsingArkFiles();

/**
 * Whether data is being read from the disc.
 *
 * The flag is one word of a block of nine boot options at 0x0070bf10 that each have an accessor of
 * this shape. The retail configurator at 0x0050f030 (InitIop() calls it first) writes the block
 * in one pass and sets this word to 1 in the same instruction run that sets GetHostMode() to
 * kHostModeCdOnly and UsingArkFiles() to 1. The `.data` default is zero, the host-development
 * configuration. It is a separate word from the one UsingArkFiles() reads at
 * 0x0070bf14.
 *
 * Both uses agree with that reading. InitAsync() starts the worker thread only when this reports
 * the disc, because a host-link read needs no latency hiding, and OpenArkObject::Open() searches
 * the path for a device prefix only then, because a prefix such as `cdrom0:` does not exist on any
 * host path.
 *
 * @return Non-zero when data is read from the disc.
 * @ghidraAddress NTSC-U/C: 0x0050efd0
 * @ghidraAddress PAL: 0x0054e548
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
 * @ghidraAddress NTSC-U/C: 0x0050eff0
 * @ghidraAddress PAL: 0x0054e568
 */
int MidiErrorLogEnabled();

/**
 * Whether Warn() reports anything.
 *
 * Another word of the same boot-option block, at 0x0070bf18. Warn() is the only reader of either
 * the word or this accessor, and it does not report anything unless the value is 1. The test
 * against 1 fixes the meaning of the word. The `.data` value is 1, and ConfigureRetailBoot() clears
 * it.
 *
 * @return 1 when warnings are reported.
 * @ghidraAddress NTSC-U/C: 0x0050efb0
 * @ghidraAddress PAL: 0x0054e528
 */
int WarningsEnabled();

/**
 * Whether a reported message also goes to the screen.
 *
 * Another word of the same boot-option block, at 0x0070bf1c. ReportMessage() is the only reader of
 * either the word or this accessor, and it skips the on-screen half unless the value is 1. No
 * routine writes the word, and its `.data` value is 1.
 *
 * @return 1 when messages go to the screen.
 * @ghidraAddress NTSC-U/C: 0x0050efc0
 * @ghidraAddress PAL: 0x0054e538
 */
int ScreenMessagesEnabled();

/**
 * Root the game composes every data path against.
 *
 * The shipped build returns the empty string. Every composed path is therefore relative. The
 * routine shares a translation unit with GetHostMode() and UsingArkFiles().
 *
 * @return The root, empty in the shipped build.
 * @ghidraAddress NTSC-U/C: 0x0050ef30
 * @ghidraAddress PAL: 0x0054e4a8
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
 * @ghidraAddress NTSC-U/C: 0x0050d9f0
 * @ghidraAddress PAL: 0x0054cea8
 */
HxStr MakeFreqPath(const HxStr &name);

/**
 * Whether a pressed modifier turns a controller button into a debug script hook.
 *
 * The boot-option word at 0x0070bf24. InputPoller::Poll() is the one reader. While it reports
 * non-zero, a button pressed with the modifier bits runs script template 0x3f2 instead of the
 * usual `'joy '` message. ConfigureRetailBoot() clears it. The name is inferred.
 *
 * @return Non-zero when the debug hooks are active.
 * @ghidraAddress NTSC-U/C: 0x0050efe0
 * @ghidraAddress PAL: 0x0054e558
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
 * @ghidraAddress NTSC-U/C: 0x0050f000
 * @ghidraAddress PAL: 0x0054e578
 */
int MemAccountingEnabled();

/**
 * Whether the start-up sequence plays the intro movie `ps2intro.pss`.
 *
 * The boot-option word at 0x0070bf30. MetSonyScreen is the one reader. ConfigureRetailBoot()
 * sets it. The name is inferred.
 *
 * @return Non-zero when the intro movie plays.
 * @ghidraAddress NTSC-U/C: 0x0050f010
 * @ghidraAddress PAL: 0x0054e588
 */
int IntroMovieEnabled();

#ifdef VIDEO_STANDARD_PAL
/**
 * Report the language the text and fonts are chosen by.
 *
 * The value is a system-configuration language code (English 1, French 2, Spanish 3, German 4,
 * Italian 5). It starts at French, and InitBootConfig() replaces it with the console's setting.
 * The European release added the routine. The name is inferred.
 *
 * @return The language code.
 * @ghidraAddress PAL: 0x0054e5a8
 */
int GetLanguage();

/**
 * Record the language the text and fonts are chosen by.
 *
 * The `set_lang` script command and InitBootConfig() are the writers. The European release added
 * the routine. The name is inferred.
 *
 * @param nLanguage The language code.
 * @ghidraAddress PAL: 0x0054e5b8
 */
void SetLanguage(int nLanguage);

/**
 * Report the suffix of the font containers for GetLanguage().
 *
 * English and an unknown code have no suffix. Each loader of a font container inlines the
 * choice. The name is inferred.
 *
 * @return `_fr`, `_sp`, `_ger`, `_it`, or an empty string.
 */
inline const char *GetFontLanguageSuffix() {
    switch (GetLanguage()) {
    case SCE_FRENCH_LANGUAGE:
        return "_fr";
    case SCE_SPANISH_LANGUAGE:
        return "_sp";
    case SCE_GERMAN_LANGUAGE:
        return "_ger";
    case SCE_ITALIAN_LANGUAGE:
        return "_it";
    default:
        return "";
    }
}
#endif

/**
 * Report the build version, "198" in the NTSC-U/C release and "197" in the PAL release.
 *
 * The title screen shows it after "Version:". The unit's static initialiser builds the string, the
 * HxStr at 0x0070bf38 (PAL 0x0074fac8). The name is inferred.
 *
 * @return A copy of the version string.
 * @ghidraAddress NTSC-U/C: 0x0050ef60
 * @ghidraAddress PAL: 0x0054e4d8
 */
HxStr GetVersionString();

/**
 * Report whether a file can be opened for reading through the file service.
 *
 * The file is opened read-only and closed again at once. The shipped program has no call site,
 * and the name is inferred.
 *
 * @param pszPath The path to test.
 * @return True when the open succeeded.
 * @ghidraAddress NTSC-U/C: 0x0050f0c8
 * @ghidraAddress PAL: 0x0054e670
 */
bool FileExists(const char *pszPath);

/**
 * Release every zone, the counterpart of InitBootConfig().
 *
 * The body is one call to ReleaseAllZoneSlots(). The shipped program has no call site, and the
 * name is inferred from the counterpart.
 *
 * @ghidraAddress NTSC-U/C: 0x0050f0a8
 * @ghidraAddress PAL: 0x0054e650
 */
void TerminateBootConfig();
