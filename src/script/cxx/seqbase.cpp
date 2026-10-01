#include "script/cxx/seqbase.h"

#include "os/hxstr.h"
#include "script/cxx/char.h"

namespace Py {

// 0x004c73d0
template <>
int SeqBase<Char>::size() const {
    return PyString_Size(mPtr);
}

// 0x004c7260
template <>
int SeqBase<Char>::max_size() const {
    return static_cast<int>(g_nHxStrNoPosition);
}

// 0x004c7890
template <>
void SeqBase<Char>::swap(SeqBase<Char> &other) {
    const SeqBase<Char> temp(other);
    if (other.mPtr != mPtr) {
        other.set(mPtr);
    }
    set(temp.mPtr);
}

// 0x0012b358
template <>
int SeqBase<Object>::size() const {
    return PySequence_Size(mPtr);
}

// 0x0012ad48
template <>
int SeqBase<Object>::max_size() const {
    return static_cast<int>(g_nHxStrNoPosition);
}

// 0x0012b3a0
template <>
void SeqBase<Object>::swap(SeqBase<Object> &other) {
    const SeqBase<Object> temp(other);
    if (other.mPtr != mPtr) {
        other.set(mPtr);
    }
    set(temp.mPtr);
}

} // namespace Py
