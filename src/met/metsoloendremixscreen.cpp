#include "met/metsoloendremixscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "erss";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "end_remix";

// The two textures the first pair flips between, and the two the second flips between.
static const char *const kSongLogoFirstTexture = "gSongLogo1.tex";
static const char *const kSongLogoSecondTexture = "gSongLogo2.tex";
static const char *const kSongLabelFirstTexture = "gSongLabel1.tex";
static const char *const kSongLabelSecondTexture = "gSongLabel2.tex";

// This screen's own registry key, and the key of the panel slot 7 activates.
static const char *const kOwnScreenName = "MetSoloEndRemixScreen";
static const char *const kSaveScreenName = "MetSaveRemixScreen";

} // namespace

MetSoloEndRemixScreen::MetSoloEndRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknownbc(HxStr(kSongLogoFirstTexture), HxStr(kSongLogoSecondTexture)),
      mUnknownec(HxStr(kSongLabelFirstTexture), HxStr(kSongLabelSecondTexture)) {
}

MetSoloEndRemixScreen::~MetSoloEndRemixScreen() {
}

void MetSoloEndRemixScreen::OnUnknownSlot7() {
    ActivateNamedPanel(HxStr(kSaveScreenName));
}

void MetSoloEndRemixScreen::OnUnknownSlot36() {
    if (mUnknownb8 == 0) {
        ReturnToTitle();
    }
}

void MetSoloEndRemixScreen::OnUnknownSlot2() {
    if (mUnknownb8 == 0) {
        BeginExit();
    } else {
        ReturnToTitle();
    }
}

void MetSoloEndRemixScreen::OnUnknownSlot3() {
    mUnknownb8 = 1;
    BeginExit();
}

void MetSoloEndRemixScreen::OnUnknownSlot4(int nFlag) {
    // The binary negates with `xori` against 1, which is what a bool argument compiles to, so only
    // 0 and 1 round-trip through the member.
    mUnknownb8 = nFlag ^ 1;
    if (nFlag != 0) {
        PushNamedScreen(HxStr(kOwnScreenName));
    } else {
        ExitScreenByName(HxStr(kOwnScreenName));
    }
}
