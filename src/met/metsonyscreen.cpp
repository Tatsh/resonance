#include "met/metsonyscreen.h"

// 0x003bd720
MetSonyScreen *MetSonyScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSonyScreen(pRenderer, nPriority);
}
