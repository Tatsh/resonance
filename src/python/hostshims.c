// Host shims the port's library subset needs but the SDK does not provide.
//
// The complex accessors repeat upstream complexobject.c without moving a
// Py_complex by value, which the console backend cannot reload. The type is
// a minimal tag for the objects the patched constructors build; the scripts
// never call into its slots. The terminal query always fails, because the
// console has no controlling terminal for the descriptor to name.

#include <Python.h>

double PyComplex_RealAsDouble(PyObject *pObject) {
    if (PyComplex_Check(pObject)) {
        return ((PyComplexObject *)pObject)->cval.real;
    }
    return PyFloat_AsDouble(pObject);
}

double PyComplex_ImagAsDouble(PyObject *pObject) {
    if (PyComplex_Check(pObject)) {
        return ((PyComplexObject *)pObject)->cval.imag;
    }
    return 0.0;
}

PyTypeObject PyComplex_Type = {
    PyObject_HEAD_INIT(&PyType_Type) 0,
    "complex",
    sizeof(PyComplexObject),
    0,
};

char *ttyname(int nDescriptor) {
    (void)nDescriptor;
    return NULL;
}
