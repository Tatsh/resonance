#include "met/metpersonasaverscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "dlg";
// The directory the container loads from. The capital S is what the image records.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "dialogue";

// mUnknownc0, mUnknowncc, and mUnknownd0 as the constructor leaves them.
constexpr int kNoSelection = -1;

} // namespace

MetPersonaSaverScreen::MetPersonaSaverScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown98(0), mUnknown9c(0), mUnknownbc(0), mUnknownc0(kNoSelection), mUnknownc4("") {
    mUnknownd0 = kNoSelection;
    mUnknownd4 = 0;
    mUnknowncc = kNoSelection;
}

MetPersonaSaverScreen::~MetPersonaSaverScreen() {
    ClearPersonas();
}

void MetPersonaSaverScreen::ClearPersonas() {
    // Yes, the binary re-reads the size on every iteration rather than caching it.
    for (unsigned index = 0; index < mPersonas.size(); ++index) {
        delete mPersonas[index];
    }
    mPersonas.clear();
}
