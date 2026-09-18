#include "rnd/transformable.h"

#include <string.h>

namespace Rnd {

// Inlined at 0x00482c20 in Rnd::Mesh::Replace.
void Transformable::AdoptXfmFrom(const Transformable &owner) {
    memcpy(mLocalXfm, owner.mWorldXfm, sizeof(mLocalXfm));
    mDirty = 1;
    UpdateWorldXfm(nullptr, 0);

    memcpy(mLocalXfm, owner.mLocalXfm, sizeof(mLocalXfm));
    mDirty = 1;
    SetBillboard(owner.mBillboard);
    SetOrigin(owner.mOrigin);
    UpdateWorldXfm(nullptr, 0);
}

} // namespace Rnd
