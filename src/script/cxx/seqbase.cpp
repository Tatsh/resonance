#include "script/cxx/seqbase.h"

#include "os/hxstr.h"
#include "script/cxx/char.h"

namespace Py {

// NTSC-U/C: 0x004c73d0, PAL: 0x005055f8
template <>
int SeqBase<Char>::size() const {
    return PyString_Size(mPtr);
}

// NTSC-U/C: 0x004c7260, PAL: 0x00505488
template <>
int SeqBase<Char>::max_size() const {
    return static_cast<int>(g_nHxStrNoPosition);
}

// NTSC-U/C: 0x004c7890, PAL: 0x00505ab8
template <>
void SeqBase<Char>::swap(SeqBase<Char> &other) {
    const SeqBase<Char> temp(other);
    if (other.mPtr != mPtr) {
        other.set(mPtr);
    }
    set(temp.mPtr);
}

// NTSC-U/C: 0x0012b358, PAL: 0x0012ba90
template <>
int SeqBase<Object>::size() const {
    return PySequence_Size(mPtr);
}

// NTSC-U/C: 0x0012ad48, PAL: 0x0012b480
template <>
int SeqBase<Object>::max_size() const {
    return static_cast<int>(g_nHxStrNoPosition);
}

// NTSC-U/C: 0x0012b3a0, PAL: 0x0012bad8
template <>
void SeqBase<Object>::swap(SeqBase<Object> &other) {
    const SeqBase<Object> temp(other);
    if (other.mPtr != mPtr) {
        other.set(mPtr);
    }
    set(temp.mPtr);
}

} // namespace Py
