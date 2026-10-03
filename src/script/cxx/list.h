#pragma once

#include "script/cxx/config.h"
#include "script/cxx/exception.h"
#include "script/cxx/fromapi.h"
#include "script/cxx/object.h"
#include "script/cxx/seqbase.h"

namespace Py {

/**
 * Handle on a Python list.
 *
 * Its RTTI descriptor is at `0x00902180`. It has `Py::SeqBase<Py::Object>` at offset 0 as its one
 * base.
 *
 * The MetHelpScreen translation unit carries a private copy of the nine-entry vtable at
 * `0x00802560`, with the type function at `0x00316f20`. Against the `SeqBase<Object>` table it
 * differs in accepts() and capacity(), the two overrides declared below at that unit's copies.
 */
class List : public SeqBase<Object> {
public:
    /**
     * Create a list of a given length.
     *
     * Inline, built as Py::Tuple's sized constructor is. The base starts on an empty tuple, the
     * body replaces it with a new list through set(), and every slot is filled with `None`.
     * PythonExtension::getattr_methods() at `0x005ad5e0` expands it with the default length.
     *
     * @param nSize The number of elements.
     */
    explicit List(int nSize = 0) : SeqBase<Object>(FromAPI(PyTuple_New(0)).mPtr) {
        set(FromAPI(PyList_New(nSize)).mPtr);
        validate();
        for (int i = 0; i < nSize; ++i) {
            Py_INCREF(Py_None);
            if (PyList_SetItem(mPtr, i, Py_None) == -1) {
                throw Exception();
            }
        }
    }

    /**
     * Take another handle's reference as a list.
     *
     * Inline. MetHelpScreen::FillTexts() at `0x00313208` expands it as three vptr stores, each
     * followed by validate().
     *
     * @param ob The handle to copy.
     */
    explicit List(const Object &ob) : SeqBase<Object>(ob) {
        validate();
    }

    /**
     * Accept only a list.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a list.
     * @ghidraAddress NTSC-U/C: 0x00317100
     * @ghidraAddress PAL: 0x005f0b78
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyList_Check(pyob);
    }

    /**
     * Report the sentinel rather than the length, as Py::String does.
     *
     * @return The value max_size() reports.
     * @ghidraAddress NTSC-U/C: 0x003170d8
     * @ghidraAddress PAL: 0x005f0b50
     */
    virtual int capacity() const {
        return max_size();
    }

    /**
     * Append one element.
     *
     * Inline. PythonExtension::getattr_methods() at `0x005ad5e0` expands it.
     *
     * @param ob The element.
     */
    void append(const Object &ob) {
        if (PyList_Append(mPtr, ob.mPtr) == -1) {
            throw Exception();
        }
    }
};

} // namespace Py
