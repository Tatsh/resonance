#include "app/attachment.h"

// 0x004bfe30
Attachment::~Attachment() {
}

// 0x004bfe60
int Attachment::Release() {
    if (mRefs != 1) {
        --mRefs;
        return mRefs;
    }
    Destroy();
    return 0;
}

// 0x004bfea8
void Attachment::Destroy() {
    // The binary really tests the receiver against null before dispatching the destructor.
    if (this != nullptr) {
        delete this;
    }
}
