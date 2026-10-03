#include "memcard/memcardsavepaths.h"

#include "os/hxstr.h"

char g_abRemixStagingBuffer[kRemixStagingBufferSize];

#ifdef VIDEO_STANDARD_PAL
const HxStr g_saveDirBase("/BESCES-50791");
#else
const HxStr g_saveDirBase("/BASCUS-97125");
#endif
const HxStr g_personasDirSuffix("gen");
const HxStr g_globalSettingsDirSuffix("glo");
const HxStr g_jukeboxDirSuffix("juk");
const HxStr g_remixDirSuffix("r");
#ifdef VIDEO_STANDARD_PAL
const HxStr g_remixDirBase("/BESCES-50791r");
const HxStr g_personasFileName("pers.dat");
const HxStr g_globalSettingsFileName("globset.dat");
const HxStr g_jukeboxFileName("jukebox");
const HxStr g_iconSysFileName("icon.sys");
const HxStr g_iconImageFileName("freq1.ico");
#else
const HxStr g_remixDirBase("/BASCUS-97125r");
const HxStr g_personasFileName("/pers.dat");
const HxStr g_globalSettingsFileName("/globset.dat");
const HxStr g_jukeboxFileName("/jukebox");
#endif
const HxStr g_personasIconTitle("Frequency - FreQs");
const HxStr g_globalSettingsIconTitle("Frequency - Global Settings");
#ifdef VIDEO_STANDARD_PAL
const HxStr g_remixIconTitle("Frequency - Remixes");
const HxStr g_jukeboxIconTitle("Frequency - Jukebox");
#else
const HxStr g_remixIconTitle("Frequency - Remixes ");
const HxStr g_jukeboxIconTitle("Frequency - Jukebox Playlists");
#endif
