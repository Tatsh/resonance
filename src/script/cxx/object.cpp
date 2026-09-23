#include "script/cxx/object.h"

#include "script/cxx/fromapi.h"
#include "script/cxx/string.h"

namespace Py {

// 0x0055c980
// Released PyCXX defines this member at the bottom of the same header, after
// Py::String becomes complete. Every class here has a header of its own, so the definition moves
// out of line instead.
String Object::str() const {
    return String(FromAPI(PyObject_Str(mPtr)).mPtr);
}

// 0x0055cb30
HxStr Object::as_string() const {
    return static_cast<HxStr>(str());
}

} // namespace Py
