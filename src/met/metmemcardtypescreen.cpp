#include "met/metmemcardtypescreen.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcrf";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "mcrf_load";

// The two container objects the screen registers, in the order the constructor pushes them.
static const char *const kRemixObjectName = "mcrf_remix";
static const char *const kFreqObjectName = "mcrf_freq";

// mUnknown90, mUnknown9c, and mUnknowna0 as the constructor leaves them.
constexpr int kNoSelection = -1;

} // namespace

MetMemCardTypeScreen::MetMemCardTypeScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(nullptr), mUnknown90(kNoSelection), mUnknown94(""), mUnknown9c(kNoSelection),
      mUnknowna0(kNoSelection), mUnknowna4(0) {
    mUnknown38.push_back(HxStr(kRemixObjectName));
    mUnknown38.push_back(HxStr(kFreqObjectName));
}

MetMemCardTypeScreen::~MetMemCardTypeScreen() {
    delete mUnknown8c;
}
