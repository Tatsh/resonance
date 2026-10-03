#include "script/cxx/pythonextensionbase.h"

#include "os/hxstr.h"
#include "script/cxx/exception.h"
#include "script/cxx/runtimeerror.h"

namespace Py {

// NTSC-U/C: 0x005aa420, PAL: 0x005ec930
void PythonExtensionBase::missing_method() {
    throw RuntimeError(HxStr("Extension object missing a required method."));
}

// NTSC-U/C: 0x005ac880, PAL: 0x005eeda8
int PythonExtensionBase::print(FILE *, int) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005ac8a0, PAL: 0x005eedc8
Object PythonExtensionBase::getattr(const char *) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ac900, PAL: 0x005eee28
int PythonExtensionBase::setattr(const char *, const Object &) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005ac920, PAL: 0x005eee48
Object PythonExtensionBase::getattro(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ac980, PAL: 0x005eeea8
int PythonExtensionBase::setattro(const Object &, const Object &) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005ac9a0, PAL: 0x005eeec8
int PythonExtensionBase::compare(const Object &) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005ac9c0, PAL: 0x005eeee8
Object PythonExtensionBase::repr() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005aca20, PAL: 0x005eef48
Object PythonExtensionBase::str() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005aca80, PAL: 0x005eefa8
Py_LONG PythonExtensionBase::hash() {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005acaa0, PAL: 0x005eefc8
Object PythonExtensionBase::call(const Object &, const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acb00, PAL: 0x005ef028
int PythonExtensionBase::sequence_length() {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005acb20, PAL: 0x005ef048
Object PythonExtensionBase::sequence_concat(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acb80, PAL: 0x005ef0a8
Object PythonExtensionBase::sequence_repeat(int) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acbe0, PAL: 0x005ef108
Object PythonExtensionBase::sequence_item(int) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acc40, PAL: 0x005ef168
Object PythonExtensionBase::sequence_slice(int, int) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acca0, PAL: 0x005ef1c8
int PythonExtensionBase::sequence_ass_item(int, const Object &) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005accc0, PAL: 0x005ef1e8
int PythonExtensionBase::sequence_ass_slice(int, int, const Object &) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005acce0, PAL: 0x005ef208
int PythonExtensionBase::mapping_length() {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005acd00, PAL: 0x005ef228
Object PythonExtensionBase::mapping_subscript(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acd60, PAL: 0x005ef288
int PythonExtensionBase::mapping_ass_subscript(const Object &, const Object &) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005acd80, PAL: 0x005ef2a8
int PythonExtensionBase::number_nonzero() {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005acda0, PAL: 0x005ef2c8
Object PythonExtensionBase::number_negative() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ace00, PAL: 0x005ef328
Object PythonExtensionBase::number_positive() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ace60, PAL: 0x005ef388
Object PythonExtensionBase::number_absolute() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acec0, PAL: 0x005ef3e8
Object PythonExtensionBase::number_invert() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acf20, PAL: 0x005ef448
Object PythonExtensionBase::number_int() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acf80, PAL: 0x005ef4a8
Object PythonExtensionBase::number_float() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005acfe0, PAL: 0x005ef508
Object PythonExtensionBase::number_long() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad040, PAL: 0x005ef568
Object PythonExtensionBase::number_oct() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad0a0, PAL: 0x005ef5c8
Object PythonExtensionBase::number_hex() {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad100, PAL: 0x005ef628
Object PythonExtensionBase::number_add(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad160, PAL: 0x005ef688
Object PythonExtensionBase::number_subtract(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad1c0, PAL: 0x005ef6e8
Object PythonExtensionBase::number_multiply(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad220, PAL: 0x005ef748
Object PythonExtensionBase::number_divide(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad280, PAL: 0x005ef7a8
Object PythonExtensionBase::number_remainder(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad2e0, PAL: 0x005ef808
Object PythonExtensionBase::number_divmod(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad340, PAL: 0x005ef868
Object PythonExtensionBase::number_lshift(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad3a0, PAL: 0x005ef8c8
Object PythonExtensionBase::number_rshift(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad400, PAL: 0x005ef928
Object PythonExtensionBase::number_and(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad460, PAL: 0x005ef988
Object PythonExtensionBase::number_xor(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad4c0, PAL: 0x005ef9e8
Object PythonExtensionBase::number_or(const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad520, PAL: 0x005efa48
Object PythonExtensionBase::number_power(const Object &, const Object &) {
    missing_method();
    return Object();
}

// NTSC-U/C: 0x005ad580, PAL: 0x005efaa8
int PythonExtensionBase::buffer_getreadbuffer(int, void **) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005ad5a0, PAL: 0x005efac8
int PythonExtensionBase::buffer_getwritebuffer(int, void **) {
    missing_method();
    return 0;
}

// NTSC-U/C: 0x005ad5c0, PAL: 0x005efae8
int PythonExtensionBase::buffer_getsegcount(int *) {
    missing_method();
    return 0;
}

// The handlers below report a thrown error as 0 rather than -1, as the binary does.

// NTSC-U/C: 0x005ac3c0, PAL: 0x005ee8e8
int PythonExtensionBase::print_handler(PyObject *self, FILE *pFile, int nFlags) {
    try {
        return static_cast<PythonExtensionBase *>(self)->print(pFile, nFlags);
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005ac458, PAL: 0x005ee980
Py_LONG PythonExtensionBase::hash_handler(PyObject *self) {
    try {
        return static_cast<PythonExtensionBase *>(self)->hash();
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005ac4f0, PAL: 0x005eea18
int PythonExtensionBase::sequence_length_handler(PyObject *self) {
    try {
        return static_cast<PythonExtensionBase *>(self)->sequence_length();
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005ac588, PAL: 0x005eeab0
int PythonExtensionBase::mapping_length_handler(PyObject *self) {
    try {
        return static_cast<PythonExtensionBase *>(self)->mapping_length();
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005ac620, PAL: 0x005eeb48
int PythonExtensionBase::number_nonzero_handler(PyObject *self) {
    try {
        return static_cast<PythonExtensionBase *>(self)->number_nonzero();
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005ac6b8, PAL: 0x005eebe0
int PythonExtensionBase::buffer_getreadbuffer_handler(PyObject *self, int nSegment, void **ppData) {
    try {
        return static_cast<PythonExtensionBase *>(self)->buffer_getreadbuffer(nSegment, ppData);
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005ac750, PAL: 0x005eec78
int PythonExtensionBase::buffer_getwritebuffer_handler(PyObject *self,
                                                       int nSegment,
                                                       void **ppData) {
    try {
        return static_cast<PythonExtensionBase *>(self)->buffer_getwritebuffer(nSegment, ppData);
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005ac7e8, PAL: 0x005eed10
int PythonExtensionBase::buffer_getsegcount_handler(PyObject *self, int *pnLength) {
    try {
        return static_cast<PythonExtensionBase *>(self)->buffer_getsegcount(pnLength);
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005a7258, PAL: 0x005e9768
PyObject *PythonExtensionBase::sequence_concat_handler(PyObject *self, PyObject *other) {
    try {
        const Object operand(other);
        const Object result = static_cast<PythonExtensionBase *>(self)->sequence_concat(operand);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

// NTSC-U/C: 0x005a7460, PAL: 0x005e9970
PyObject *PythonExtensionBase::sequence_repeat_handler(PyObject *self, int nCount) {
    try {
        const Object result = static_cast<PythonExtensionBase *>(self)->sequence_repeat(nCount);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

// NTSC-U/C: 0x005a75a8, PAL: 0x005e9ab8
PyObject *PythonExtensionBase::sequence_item_handler(PyObject *self, int nIndex) {
    try {
        const Object result = static_cast<PythonExtensionBase *>(self)->sequence_item(nIndex);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

// NTSC-U/C: 0x005a76f0, PAL: 0x005e9c00
PyObject *PythonExtensionBase::sequence_slice_handler(PyObject *self, int nFirst, int nLast) {
    try {
        const Object result =
            static_cast<PythonExtensionBase *>(self)->sequence_slice(nFirst, nLast);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

// NTSC-U/C: 0x005a7838, PAL: 0x005e9d48
int PythonExtensionBase::sequence_ass_item_handler(PyObject *self, int nIndex, PyObject *value) {
    try {
        const Object item(value);
        return static_cast<PythonExtensionBase *>(self)->sequence_ass_item(nIndex, item);
    } catch (Exception &) {
        return 0;
    }
}

// NTSC-U/C: 0x005a79b8, PAL: 0x005e9ec8
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

// NTSC-U/C: 0x005a7b48, PAL: 0x005ea058
PyObject *PythonExtensionBase::mapping_subscript_handler(PyObject *self, PyObject *key) {
    try {
        const Object index(key);
        const Object result = static_cast<PythonExtensionBase *>(self)->mapping_subscript(index);
        return new_reference_to(result);
    } catch (Exception &) {
        return nullptr;
    }
}

// NTSC-U/C: 0x005a7d50, PAL: 0x005ea260
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
