#include "met/metmemdetectscreen.h"

MetMemDetectScreen::MetMemDetectScreen(MetRenderer *pRenderer,
                                       int nPriority,
                                       const HxStr &name,
                                       const HxStr &directory,
                                       const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mUnknown90(0), mUnknown94(0),
      mUnknown98(0), mUnknown9c(0) {
}
