#include "script/cxx/object.h"

#include "script/cxx/fromapi.h"
#include "script/cxx/string.h"
#include "script/cxx/type.h"

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

// 0x0055c818
Type Object::type() const {
    return Type(FromAPI(PyObject_Type(mPtr)).mPtr);
}

// 0x0055ceb0
bool Object::isType(const Type &type) const {
    return this->type().mPtr == type.mPtr;
}

// 0x004c3f80
Object Object::getAttr(const HxStr &name) const {
    FromAPI holder(
        PyObject_GetAttrString(mPtr, const_cast<char *>(name.mStr != nullptr ? name.mStr : "")));
    Object result;
    result.mPtr = holder.mPtr;
    Py_XINCREF(result.mPtr);
    result.validate();
    return result;
}

} // namespace Py
