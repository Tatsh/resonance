#include "script/cxx/string.h"

#include "script/cxx/fromapi.h"

namespace Py {

// 0x004c4d88
String::String(const HxStr &text) {
    // A null handle falls back to an empty string. The image reads the fallback through a global
    // whose stable value was not recovered, so the reconstruction uses the literal directly.
    FromAPI holder(PyString_FromString(text.mStr != nullptr ? text.mStr : ""));
    mPtr = holder.mPtr;
    Py_XINCREF(mPtr);
    validate();
    validate();
}

// 0x004c4c60
String::String(const Object &ob) {
    mPtr = ob.mPtr;
    Py_XINCREF(mPtr);
    validate();
    validate();
}

} // namespace Py
