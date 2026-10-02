#pragma once

#include "os/hxstr.h"

/**
 * Bytes of the one shared staging buffer every remix task streams through.
 *
 * @ghidraAddress 0x008f2aa0
 */
constexpr int kRemixStagingBufferSize = 0xf000;

/**
 * Staging buffer the four remix tasks and SavePersonasMCT all build their payload in.
 *
 * One buffer serves every task, so two tasks running at once would overwrite each other. Each task
 * wraps it in an IOBPreallocMemStream of its own at construction.
 *
 * @ghidraAddress 0x008f2aa0
 */
extern char g_abRemixStagingBuffer[kRemixStagingBufferSize];

/**
 * Save directory every card path this game writes begins with.
 *
 * The thirteen strings declared here are one file-scope constant group, built by the static
 * initialisation stub at `0x00183028` in the order they appear below. The group belongs to the one
 * translation unit that defined all sixteen MemcardTask subclasses, and this header exists because
 * the reconstruction splits that unit one class per file.
 *
 * @ghidraAddress 0x00888940
 */
extern const HxStr g_saveDirBase;

/** Directory suffix for the FreQ roster. @ghidraAddress 0x00888948 */
extern const HxStr g_personasDirSuffix;

/** Directory suffix for the global settings. @ghidraAddress 0x00888950 */
extern const HxStr g_globalSettingsDirSuffix;

/** Directory suffix for a jukebox playlist. @ghidraAddress 0x00888958 */
extern const HxStr g_jukeboxDirSuffix;

/** Directory suffix for a remix. @ghidraAddress 0x00888960 */
extern const HxStr g_remixDirSuffix;

/** g_saveDirBase and g_remixDirSuffix as one literal. @ghidraAddress 0x00888968 */
extern const HxStr g_remixDirBase;

/** Payload file holding the FreQ roster. @ghidraAddress 0x00888970 */
extern const HxStr g_personasFileName;

/** Payload file holding the global settings. @ghidraAddress 0x00888978 */
extern const HxStr g_globalSettingsFileName;

/** Payload file holding a jukebox playlist. @ghidraAddress 0x00888980 */
extern const HxStr g_jukeboxFileName;

/** Browser title for a roster save. @ghidraAddress 0x00888988 */
extern const HxStr g_personasIconTitle;

/** Browser title for a settings save. @ghidraAddress 0x00888990 */
extern const HxStr g_globalSettingsIconTitle;

/** Browser title for a remix save, which the directory number is appended to. @ghidraAddress
 *  0x00888998 */
extern const HxStr g_remixIconTitle;

/** Browser title for a playlist save. @ghidraAddress 0x008889a0 */
extern const HxStr g_jukeboxIconTitle;
