#include "app/attachment.h"

Attachment::~Attachment() {
}

int Attachment::Release() {
    if (mRefs != 1) {
        --mRefs;
        return mRefs;
    }
    Destroy();
    return 0;
}

void Attachment::Destroy() {
    // The binary really tests the receiver against null before dispatching the destructor. The
    // standard makes `this` non-null, so a current compiler both warns and is free to discard the
    // test. The test is retained because the original performs it, and the diagnostic is
    // suppressed here rather than over the file.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
    if (this != nullptr) {
        delete this;
    }
#pragma GCC diagnostic pop
}
