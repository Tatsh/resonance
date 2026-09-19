#include "met/metloadfreqscreen.h"

namespace {

// mUnknowna8 as the constructor leaves it.
constexpr int kInitialUnknowna8 = 1;

} // namespace

MetLoadFreqScreen::MetLoadFreqScreen(MetRenderer *pRenderer, int nPriority)
    : MetLoadFreqBaseScreen(pRenderer, nPriority) {
    mUnknowna8 = kInitialUnknowna8;
}
