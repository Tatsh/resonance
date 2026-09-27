#include "script/cxx/seqbase.h"

#include "os/hxstr.h"
#include "script/cxx/char.h"

namespace Py {

// 0x0012b358
template <>
int SeqBase<Char>::size() const {
    return PyString_Size(mPtr);
}

// 0x0012ad48
template <>
int SeqBase<Char>::max_size() const {
    return static_cast<int>(g_nHxStrNoPosition);
}

// 0x0012b3a0
template <>
void SeqBase<Char>::swap(SeqBase<Char> &other) {
    PyObject *temp = mPtr;
    mPtr = other.mPtr;
    other.mPtr = temp;
}

} // namespace Py
