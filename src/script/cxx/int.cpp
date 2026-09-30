#include "script/cxx/int.h"

#include "script/cxx/fromapi.h"

namespace Py {

// 0x004c4680
Int::Int(const Object &ob) {
    FromAPI holder(PyNumber_Int(ob.mPtr));
    mPtr = holder.mPtr;
    Py_XINCREF(mPtr);
    validate();
    validate();
}

// 0x004c44c0
Int::Int(long nValue) {
    FromAPI holder(PyInt_FromLong(nValue));
    set(holder.mPtr);
}

// 0x004c7240
Int::operator long() const {
    return PyInt_AsLong(mPtr);
}

} // namespace Py
