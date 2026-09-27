#include "script/cxx/tuple.h"

#include "script/cxx/fromapi.h"

namespace Py {

// 0x004c5690
Tuple::Tuple(int nSize) {
    // The image fills the slots through helpers the reconstruction has not recovered. Prefilling
    // with None keeps every slot owned, which a bare new tuple does not guarantee.
    FromAPI holder(PyTuple_New(nSize));
    mPtr = holder.mPtr;
    Py_XINCREF(mPtr);
    for (int i = 0; i < nSize; ++i) {
        Py_INCREF(Py_None);
        PyTuple_SetItem(mPtr, i, Py_None);
    }
    validate();
    validate();
}

} // namespace Py
