#include "script/cxx/int.h"

#include "script/cxx/fromapi.h"

namespace Py {

Int::Int(const Object &ob) {
    set(FromAPI(PyNumber_Int(ob.mPtr)).mPtr);
    validate();
}

Int::Int(long long nValue) {
    set(FromAPI(PyInt_FromLong(nValue)).mPtr);
    validate();
}

Int &Int::operator=(const Object &ob) {
    return operator=(ob.mPtr);
}

Int &Int::operator=(PyObject *pyob) {
    if (mPtr == pyob) {
        return *this;
    }
    set(FromAPI(PyNumber_Int(pyob)).mPtr);
    return *this;
}

Int &Int::operator=(int nValue) {
    return operator=(FromAPI(PyInt_FromLong(nValue)).mPtr);
}

Int &Int::operator=(long long nValue) {
    return operator=(FromAPI(PyInt_FromLong(nValue)).mPtr);
}

Int::operator long long() const {
    return PyInt_AsLong(mPtr);
}

} // namespace Py
