#include "met/metsaveremix.h"

MetSaveRemix::MetSaveRemix(MetRenderer *pRenderer,
                           int nPriority,
                           const HxStr &name,
                           const HxStr &directory,
                           const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mUnknownc8(0), mUnknownd8(0),
      mUnknowndc(0), mUnknowne4(0) {
}

MetSaveRemix::~MetSaveRemix() {
}

void MetSaveRemix::OnUnknownSlot40() {
}

void MetSaveRemix::OnUnknownSlot41() {
}

void MetSaveRemix::OnUnknownSlot42() {
    OnUnknownSlot40();
}
