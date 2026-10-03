#pragma once

#include "os/hxstr.h"

/**
 * Bytes of the one shared staging buffer every remix task streams through.
 *
 * @ghidraAddress NTSC-U/C: 0x008f2aa0
 * @ghidraAddress PAL: 0x00937ab0
 */
constexpr int kRemixStagingBufferSize = 0xf000;

/**
 * Staging buffer the four remix tasks and SavePersonasMCT all build their payload in.
 *
 * One buffer serves every task. Two tasks running at once would overwrite each other. Each task
 * wraps the buffer in a separate IOBPreallocMemStream at construction.
 *
 * @ghidraAddress NTSC-U/C: 0x008f2aa0
 * @ghidraAddress PAL: 0x00937ab0
 */
extern char g_abRemixStagingBuffer[kRemixStagingBufferSize];

/**
 * Save directory every card path this game writes begins with.
 *
 * The thirteen strings declared here are one file-scope constant group, built by the static
 * initialisation stub at `0x00183028` in the order they appear below. The group belongs to the one
 * translation unit that defined all sixteen MemcardTask subclasses.
 *
 * The European release builds fifteen strings in its stub at `0x00188108`. Its directories include
 * the product code `BESCES-50791`, its payload file names have no leading `/`, and it adds
 * g_iconSysFileName and g_iconImageFileName.
 *
 * @ghidraAddress NTSC-U/C: 0x00888940
 * @ghidraAddress PAL: 0x008cd050
 */
extern const HxStr g_saveDirBase;

/**
 * Directory suffix for the FreQ roster.
 *
 * @ghidraAddress NTSC-U/C: 0x00888948
 * @ghidraAddress PAL: 0x008cd058
 */
extern const HxStr g_personasDirSuffix;

/**
 * Directory suffix for the global settings.
 *
 * @ghidraAddress NTSC-U/C: 0x00888950
 * @ghidraAddress PAL: 0x008cd060
 */
extern const HxStr g_globalSettingsDirSuffix;

/**
 * Directory suffix for a jukebox playlist.
 *
 * @ghidraAddress NTSC-U/C: 0x00888958
 * @ghidraAddress PAL: 0x008cd068
 */
extern const HxStr g_jukeboxDirSuffix;

/**
 * Directory suffix for a remix.
 *
 * @ghidraAddress NTSC-U/C: 0x00888960
 * @ghidraAddress PAL: 0x008cd070
 */
extern const HxStr g_remixDirSuffix;

/**
 * g_saveDirBase and g_remixDirSuffix as one literal.
 *
 * @ghidraAddress NTSC-U/C: 0x00888968
 * @ghidraAddress PAL: 0x008cd078
 */
extern const HxStr g_remixDirBase;

/**
 * Payload file with the FreQ roster.
 *
 * @ghidraAddress NTSC-U/C: 0x00888970
 * @ghidraAddress PAL: 0x008cd080
 */
extern const HxStr g_personasFileName;

/**
 * Payload file with the global settings.
 *
 * @ghidraAddress NTSC-U/C: 0x00888978
 * @ghidraAddress PAL: 0x008cd088
 */
extern const HxStr g_globalSettingsFileName;

/**
 * Payload file with a jukebox playlist.
 *
 * @ghidraAddress NTSC-U/C: 0x00888980
 * @ghidraAddress PAL: 0x008cd090
 */
extern const HxStr g_jukeboxFileName;

#ifdef VIDEO_STANDARD_PAL
/**
 * Browser icon header file SaveFileMCT writes into every save directory.
 *
 * @ghidraAddress PAL: 0x008cd098
 */
extern const HxStr g_iconSysFileName;

/**
 * Icon mesh file SaveFileMCT writes into every save directory. `icon.sys` refers to it as well.
 *
 * @ghidraAddress PAL: 0x008cd0a0
 */
extern const HxStr g_iconImageFileName;
#endif

/**
 * Browser title for a roster save.
 *
 * @ghidraAddress NTSC-U/C: 0x00888988
 * @ghidraAddress PAL: 0x008cd0a8
 */
extern const HxStr g_personasIconTitle;

/**
 * Browser title for a settings save.
 *
 * @ghidraAddress NTSC-U/C: 0x00888990
 * @ghidraAddress PAL: 0x008cd0b0
 */
extern const HxStr g_globalSettingsIconTitle;

/**
 * Browser title for a remix save. The directory number is appended to it.
 *
 * The North American title ends in a space and the European title does not.
 *
 * @ghidraAddress NTSC-U/C: 0x00888998
 * @ghidraAddress PAL: 0x008cd0b8
 */
extern const HxStr g_remixIconTitle;

/**
 * Browser title for a playlist save.
 *
 * @ghidraAddress NTSC-U/C: 0x008889a0
 * @ghidraAddress PAL: 0x008cd0c0
 */
extern const HxStr g_jukeboxIconTitle;
