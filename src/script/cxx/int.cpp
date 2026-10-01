#include "script/cxx/int.h"

#include "script/cxx/fromapi.h"

namespace Py {

// 0x004c4680
Int::Int(const Object &ob) {
    set(FromAPI(PyNumber_Int(ob.mPtr)).mPtr);
    validate();
}

// 0x004c44c0
Int::Int(long long nValue) {
    set(FromAPI(PyInt_FromLong(nValue)).mPtr);
    validate();
}

// 0x004c71f8
Int &Int::operator=(const Object &ob) {
    return operator=(ob.mPtr);
}

// 0x004c4840
Int &Int::operator=(PyObject *pyob) {
    if (mPtr == pyob) {
        return *this;
    }
    set(FromAPI(PyNumber_Int(pyob)).mPtr);
    return *this;
}

// 0x004c4980
Int &Int::operator=(int nValue) {
    return operator=(FromAPI(PyInt_FromLong(nValue)).mPtr);
}

// 0x004c4a60
Int &Int::operator=(long long nValue) {
    return operator=(FromAPI(PyInt_FromLong(nValue)).mPtr);
}

// 0x004c7240
Int::operator long long() const {
    return PyInt_AsLong(mPtr);
}

} // namespace Py
