#pragma once

/** Which media LoadIopModule() may take a module from. */
enum IopModuleSource {
    kIopModuleSourceDisc = 1, /*!< `cdrom0:\IOP\<NAME>.IRX;1`. */
    kIopModuleSourceHost = 2  /*!< `host0:iop/<name>.irx`. */
};

/**
 * One IOP module the game loads during start-up.
 *
 * The two words after the name are the argument block sceSifLoadModule() takes. The load at
 * 0x004de2f8 passes the word at +0x04 as the argument length and the word at +0x08 as the argument
 * pointer, which is what fixes both. All ten modules of the shipped table pass a zero length and a
 * null pointer, so no module receives arguments.
 */
struct IopModule {
    const char *mName;  /*!< Module name, with no path and no extension. */
    int mArgLength;     /*!< Argument block length, zero for every shipped module. */
    const char *mpArgs; /*!< Argument block, null for every shipped module. */
};

/**
 * Bring the IOP up with the game's IOP image and restart the services that depend on it.
 *
 * The retail boot configuration is written first. The IOP is then rebooted and resynchronised, and
 * the debug console and the boot configuration are initialised last. The image is
 * `cdrom0:\IOP\IOPRP23.IMG;1` in kHostModeCdOnly and `host0:iop/ioprp23.img` otherwise.
 *
 * @ghidraAddress 0x004dfe28
 */
void InitIop();

/**
 * Load one IOP module from the disc or over the host link.
 *
 * The disc arm upper-cases the module name and composes `cdrom0:\IOP\`…`.IRX;1`. The host arm
 * composes `host0:iop/`…`.irx` from the name as the table records it, with no case conversion.
 *
 * The two arms are exclusive rather than a fallback chain. kIopModuleSourceDisc wins wherever it is
 * set, so a mask with both bits never arrives at the host arm. A failed load reports through
 * Error() and calls exit(1) rather than trying the other medium.
 *
 * An empty mask reports `LoadModuleFromAnywhere failed` and calls exit(1). That message is the only
 * place the image attests a name for this routine, and the existing spelling is retained in
 * preference to it.
 *
 * @param pModule The module to load.
 * @param nSources A mask of IopModuleSource values.
 * @ghidraAddress 0x004de170
 */
void LoadIopModule(const IopModule *pModule, unsigned nSources);

/**
 * Load every IOP module the game needs and start the services that depend on them.
 *
 * The selected host mode is reported to `cout`, the SIF RPC layer and the IOP heap are initialised,
 * the ten modules of the shipped table are loaded, and the multitap and memory card libraries are
 * started. The host mode also selects the media mask. kHostModeCdHost permits both media,
 * kHostModeCdOnly permits the disc, and kHostModeHostOnly permits the host link.
 *
 * @ghidraAddress 0x004de600
 */
void LoadIopModules();

/**
 * Spin until the GS raises the start-of-vblank interrupt, then acknowledge it.
 *
 * The routine belongs to the graphics layer rather than to this translation unit, and it is
 * declared here so that iop.cpp can call it.
 *
 * @ghidraAddress 0x005963e0
 */
void WaitVsync();

/**
 * Write the retail boot options over the boot-option block.
 *
 * The routine belongs to the same translation unit as GetHostMode() and is declared here so that
 * iop.cpp can call it. It writes all nine words of the block at 0x0070bf10 in one pass, setting the
 * host mode to kHostModeCdOnly and UsingArkFiles() to 1.
 *
 * @ghidraAddress 0x0050f030
 */
void ConfigureRetailBoot();

/**
 * Bring up the graphics path and the on-screen debug console.
 *
 * The routine belongs to another translation unit and is declared here so that iop.cpp can call it.
 * That unit is devconsole.cpp. It initialises the GS through InitDebugGs() and then creates a
 * console of 75 columns by 30 rows of 16-bit character cells, clearing every cell to 0x0720, a
 * space with attribute 7.
 *
 * The name is inferred from the console geometry and the cell fill. Nothing in the image attests
 * it, and InitIop() is the only caller.
 *
 * @ghidraAddress 0x005e5f18
 */
void InitDebugConsole();

/**
 * Apply the boot configuration and build the zone list.
 *
 * The routine belongs to the same translation unit as GetHostMode() and is declared here so that
 * iop.cpp can call it. It reports ` Running from CD only, since we couldn't find the config file`
 * and ` Running from CD ONLY, forcing arkfiles ON and async ON`, writes the host mode, the ark
 * flag, and the disc flag again, opens the memory report, and then calls InitializeZoneList().
 * The shipped build reports both messages unconditionally. No build variant that shipped reads
 * the configuration file.
 *
 * The name is inferred. Nothing in the image attests it, and InitIop() is the only caller.
 *
 * @ghidraAddress 0x0050f080
 */
void InitBootConfig();

/**
 * Register the hard-effect script commands.
 *
 * The image emits the routine far from the rest of this module, after `tanf()`. It registers script
 * templates 10000 through 10011 against the `current_level` expressions for the hard effect, its
 * chorus, and the hard synth error file, with `current_level_exists()` taking 10010 out of sequence
 * between the fifth and the sixth.
 *
 * The name is inferred from those expressions. Nothing in the image attests it, and
 * LoadIopModules() is the only caller.
 *
 * @ghidraAddress 0x005e1210
 */
void RegisterHardEffectCommands();
