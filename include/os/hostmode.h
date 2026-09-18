#pragma once

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
