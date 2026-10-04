#include "script/cxx/seqbase.h"

#include "os/hxstr.h"
#include "script/cxx/char.h"

namespace Py {

template <>
int SeqBase<Char>::size() const {
    return PyString_Size(mPtr);
}

template <>
int SeqBase<Char>::max_size() const {
    return static_cast<int>(g_nHxStrNoPosition);
}

template <>
void SeqBase<Char>::swap(SeqBase<Char> &other) {
    const SeqBase<Char> temp(other);
    if (other.mPtr != mPtr) {
        other.set(mPtr);
    }
    set(temp.mPtr);
}

template <>
int SeqBase<Object>::size() const {
    return PySequence_Size(mPtr);
}

template <>
int SeqBase<Object>::max_size() const {
    return static_cast<int>(g_nHxStrNoPosition);
}

template <>
void SeqBase<Object>::swap(SeqBase<Object> &other) {
    const SeqBase<Object> temp(other);
    if (other.mPtr != mPtr) {
        other.set(mPtr);
    }
    set(temp.mPtr);
}

} // namespace Py
