#pragma once

/** Which media LoadIopModule() may take a module from. */
enum IopModuleSource {
    kIopModuleSourceDisc = 1, /*!< `cdrom0:\IOP\<NAME>.IRX;1`. */
    kIopModuleSourceHost = 2  /*!< `host0:iop/<name>.irx`. */
};

/**
 * One IOP module the game loads during start-up.
 *
 * The disc path reads only the module name. Both trailing words are zero for every module in the
 * shipped table, and their purpose has not been recovered.
 */
struct IopModule {
    const char *mName; // +0x00
    int mUnknown04;    // +0x04
    int mUnknown08;    // +0x08
};

/**
 * Reboot the IOP with the game's IOP image and wait for the reboot to finish.
 *
 * The image is `cdrom0:\IOP\IOPRP23.IMG;1` in kHostModeCdOnly and `host0:iop/ioprp23.img`
 * otherwise.
 *
 * @ghidraAddress 0x004dfe28
 */
void InitIop();

/**
 * Load one IOP module.
 *
 * The module name is upper-cased and wrapped in `cdrom0:\IOP\`…`.IRX;1` for the disc, or
 * lower-cased under `iop/` for the host link.
 *
 * @param pModule The module to load.
 * @param nSources A mask of IopModuleSource values to try.
 * @ghidraAddress 0x004de170
 */
void LoadIopModule(const IopModule *pModule, unsigned nSources);

/**
 * Load every IOP module the game needs and bring up the services that depend on them.
 *
 * @ghidraAddress 0x004de600
 */
void LoadIopModules();

/**
 * Spin until the GS raises the start-of-vblank interrupt, then acknowledge it.
 *
 * @ghidraAddress 0x005963e0
 */
void WaitVsync();
