#include "met/metlogoscreen.h"

// 0x002be390
MetLogoScreen *MetLogoScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLogoScreen(pRenderer, nPriority);
}
