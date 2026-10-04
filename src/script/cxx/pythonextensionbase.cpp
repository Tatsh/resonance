#include "script/cxx/pythonextensionbase.h"

#include "os/hxstr.h"
#include "script/cxx/exception.h"
#include "script/cxx/runtimeerror.h"

namespace Py {

void PythonExtensionBase::missing_method() {
    throw RuntimeError(HxStr("Extension object missing a required method."));
}

int PythonExtensionBase::print(FILE *, int) {
    missing_method();
    return 0;
}

Object PythonExtensionBase::getattr(const char *) {
    missing_method();
    return Object();
}

int PythonExtensionBase::setattr(const char *, const Object &) {
    missing_method();
    return 0;
}

Object PythonExtensionBase::getattro(const Object &) {
    missing_method();
    return Object();
}

int PythonExtensionBase::setattro(const Object &, const Object &) {
    missing_method();
    return 0;
}

int PythonExtensionBase::compare(const Object &) {
    missing_method();
    return 0;
}

Object PythonExtensionBase::repr() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::str() {
    missing_method();
    return Object();
}

Py_LONG PythonExtensionBase::hash() {
    missing_method();
    return 0;
}

Object PythonExtensionBase::call(const Object &, const Object &) {
    missing_method();
    return Object();
}

int PythonExtensionBase::sequence_length() {
    missing_method();
    return 0;
}

Object PythonExtensionBase::sequence_concat(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::sequence_repeat(int) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::sequence_item(int) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::sequence_slice(int, int) {
    missing_method();
    return Object();
}

int PythonExtensionBase::sequence_ass_item(int, const Object &) {
    missing_method();
    return 0;
}

int PythonExtensionBase::sequence_ass_slice(int, int, const Object &) {
    missing_method();
    return 0;
}

int PythonExtensionBase::mapping_length() {
    missing_method();
    return 0;
}

Object PythonExtensionBase::mapping_subscript(const Object &) {
    missing_method();
    return Object();
}

int PythonExtensionBase::mapping_ass_subscript(const Object &, const Object &) {
    missing_method();
    return 0;
}

int PythonExtensionBase::number_nonzero() {
    missing_method();
    return 0;
}

Object PythonExtensionBase::number_negative() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_positive() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_absolute() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_invert() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_int() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_float() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_long() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_oct() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_hex() {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_add(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_subtract(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_multiply(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_divide(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_remainder(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_divmod(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_lshift(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_rshift(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_and(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_xor(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_or(const Object &) {
    missing_method();
    return Object();
}

Object PythonExtensionBase::number_power(const Object &, const Object &) {
    missing_method();
    return Object();
}

int PythonExtensionBase::buffer_getreadbuffer(int, void **) {
    missing_method();
    return 0;
}

int PythonExtensionBase::buffer_getwritebuffer(int, void **) {
    missing_method();
    return 0;
}

int PythonExtensionBase::buffer_getsegcount(int *) {
    missing_method();
    return 0;
}

// The handlers below report a thrown error as 0 rather than -1, as the binary does.

int PythonExtensionBase::print_handler(PyObject *self, FILE *pFile, int nFlags) {
    try {
        return static_cast<PythonExtensionBase *>(self)->print(pFile, nFlags);
    } catch (Exception &) {
        return 0;
    }
}

Py_LONG PythonExtensionBase::hash_handler(PyObject *self) {
    try {
        return static_cast<PythonExtensionBase *>(self)->hash();
    } catch (Exception &) {
        return 0;
    }
}

int PythonExtensionBase::sequence_length_handler(PyObject *self) {
    try {
        return static_cast<PythonExtensionBase *>(self)->sequence_length();
    } catch (Exception &) {
        return 0;
    }
}

int PythonExtensionBase::mapping_length_handler(PyObject *self) {
    try {
        return static_cast<PythonExtensionBase *>(self)->mapping_length();
    } catch (Exception &) {
        return 0;
    }
}

int PythonExtensionBase::number_nonzero_handler(PyObject *self) {
    try {
        return static_cast<PythonExtensionBase *>(self)->number_nonzero();
    } catch (Exception &) {
        return 0;
    }
}

int PythonExtensionBase::buffer_getreadbuffer_handler(PyObject *self, int nSegment, void **ppData) {
    try {
        return static_cast<PythonExtensionBase *>(self)->buffer_getreadbuffer(nSegment, ppData);
    } catch (Exception &) {
        return 0;
    }
}

int PythonExtensionBase::buffer_getwritebuffer_handler(PyObject *self,
                                                       int nSegment,
                                                       void **ppData) {
    try {
        return static_cast<PythonExtensionBase *>(self)->buffer_getwritebuffer(nSegment, ppData);
    } catch (Exception &) {
        return 0;
    }
}

int PythonExtensionBase::buffer_getsegcount_handler(PyObject *self, int *pnLength) {
    try {
        return static_cast<PythonExtensionBase *>(self)->buffer_getsegcount(pnLength);
    } catch (Exception &) {
        return 0;
    }
}

PyObject *PythonExtensionBase::sequence_concat_handler(PyObject *self, PyObject *other) {
    try {
        const Object operand(other);
        const Object result = static_cast<PythonExtensionBase *>(self)->sequence_concat(operand);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

PyObject *PythonExtensionBase::sequence_repeat_handler(PyObject *self, int nCount) {
    try {
        const Object result = static_cast<PythonExtensionBase *>(self)->sequence_repeat(nCount);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

PyObject *PythonExtensionBase::sequence_item_handler(PyObject *self, int nIndex) {
    try {
        const Object result = static_cast<PythonExtensionBase *>(self)->sequence_item(nIndex);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

PyObject *PythonExtensionBase::sequence_slice_handler(PyObject *self, int nFirst, int nLast) {
    try {
        const Object result =
            static_cast<PythonExtensionBase *>(self)->sequence_slice(nFirst, nLast);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

int PythonExtensionBase::sequence_ass_item_handler(PyObject *self, int nIndex, PyObject *value) {
    try {
        const Object item(value);
        return static_cast<PythonExtensionBase *>(self)->sequence_ass_item(nIndex, item);
    } catch (Exception &) {
        return 0;
    }
}

int PythonExtensionBase::sequence_ass_slice_handler(PyObject *self,
                                                    int nFirst,
                                                    int nLast,
                                                    PyObject *value) {
    try {
        const Object items(value);
        return static_cast<PythonExtensionBase *>(self)->sequence_ass_slice(nFirst, nLast, items);
    } catch (Exception &) {
        return 0;
    }
}

PyObject *PythonExtensionBase::mapping_subscript_handler(PyObject *self, PyObject *key) {
    try {
        const Object index(key);
        const Object result = static_cast<PythonExtensionBase *>(self)->mapping_subscript(index);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

int PythonExtensionBase::mapping_ass_subscript_handler(PyObject *self,
                                                       PyObject *key,
                                                       PyObject *value) {
    try {
        const Object index(key);
        const Object item(value);
        return static_cast<PythonExtensionBase *>(self)->mapping_ass_subscript(index, item);
    } catch (Exception &) {
        return 0;
    }
}

} // namespace Py
