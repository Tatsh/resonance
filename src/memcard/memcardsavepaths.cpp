#include "memcard/memcardsavepaths.h"

#include "os/hxstr.h"

char g_abRemixStagingBuffer[kRemixStagingBufferSize];

const HxStr g_saveDirBase("/BASCUS-97125");
const HxStr g_personasDirSuffix("gen");
const HxStr g_globalSettingsDirSuffix("glo");
const HxStr g_jukeboxDirSuffix("juk");
const HxStr g_remixDirSuffix("r");
const HxStr g_remixDirBase("/BASCUS-97125r");
const HxStr g_personasFileName("/pers.dat");
const HxStr g_globalSettingsFileName("/globset.dat");
const HxStr g_jukeboxFileName("/jukebox");
const HxStr g_personasIconTitle("Frequency - FreQs");
const HxStr g_globalSettingsIconTitle("Frequency - Global Settings");
const HxStr g_remixIconTitle("Frequency - Remixes ");
const HxStr g_jukeboxIconTitle("Frequency - Jukebox Playlists");
