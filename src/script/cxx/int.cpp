#include "script/cxx/int.h"

#include "script/cxx/fromapi.h"

namespace Py {

// NTSC-U/C: 0x004c4680, PAL: 0x00502908
Int::Int(const Object &ob) {
    set(FromAPI(PyNumber_Int(ob.mPtr)).mPtr);
    validate();
}

// NTSC-U/C: 0x004c44c0, PAL: 0x00502748
Int::Int(long long nValue) {
    set(FromAPI(PyInt_FromLong(nValue)).mPtr);
    validate();
}

// NTSC-U/C: 0x004c71f8, PAL: 0x00505420
Int &Int::operator=(const Object &ob) {
    return operator=(ob.mPtr);
}

// NTSC-U/C: 0x004c4840, PAL: 0x00502ac8
Int &Int::operator=(PyObject *pyob) {
    if (mPtr == pyob) {
        return *this;
    }
    set(FromAPI(PyNumber_Int(pyob)).mPtr);
    return *this;
}

// NTSC-U/C: 0x004c4980, PAL: 0x00502c08
Int &Int::operator=(int nValue) {
    return operator=(FromAPI(PyInt_FromLong(nValue)).mPtr);
}

// NTSC-U/C: 0x004c4a60, PAL: 0x00502ce8
Int &Int::operator=(long long nValue) {
    return operator=(FromAPI(PyInt_FromLong(nValue)).mPtr);
}

// NTSC-U/C: 0x004c7240, PAL: 0x00505468
Int::operator long long() const {
    return PyInt_AsLong(mPtr);
}

} // namespace Py
