#include "met/metremixdatascreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcdat";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_remix_data";

// The two texture pairs the constructor builds.
static const char *const kSongLogoFirst = "gSongLogo1.tex";
static const char *const kSongLogoSecond = "gSongLogo2.tex";
static const char *const kSongLabelFirst = "gSongLabel1.tex";
static const char *const kSongLabelSecond = "gSongLabel2.tex";

} // namespace

MetRemixDataScreen::MetRemixDataScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mLogoTextures(HxStr(kSongLogoFirst), HxStr(kSongLogoSecond)),
      mLabelTextures(HxStr(kSongLabelFirst), HxStr(kSongLabelSecond)) {
}
