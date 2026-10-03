#include "script/cxx/tuple.h"

#include "script/cxx/fromapi.h"

namespace Py {

// NTSC-U/C: 0x004c5568, PAL: 0x005037f0
Tuple::Tuple(const Object &ob) : SeqBase<Object>(ob) {
    validate();
}

// NTSC-U/C: 0x004c5690, PAL: 0x00503918
Tuple::Tuple(int nSize) : SeqBase<Object>(FromAPI(PyTuple_New(0)).mPtr) {
    set(FromAPI(PyTuple_New(nSize)).mPtr);
    validate();
    for (int i = 0; i < nSize; ++i) {
        Py_INCREF(Py_None);
        if (PyTuple_SetItem(mPtr, i, Py_None) == -1) {
            throw Exception();
        }
    }
}

} // namespace Py
