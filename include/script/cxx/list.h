#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"
#include "script/cxx/seqbase.h"

namespace Py {

/**
 * Handle on a Python list.
 *
 * `Q22Py4List` in the RTTI descriptor at `0x00902180`, with `Py::SeqBase<Py::Object>` at offset 0
 * as its one base.
 *
 * The MetHelpScreen translation unit carries a private copy of the nine-entry vtable at
 * `0x00802560`, with the type function at `0x00316f20`. Against the `SeqBase<Object>` table it
 * differs in accepts() and capacity(), the two overrides declared below at that unit's copies.
 */
class List : public SeqBase<Object> {
public:
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
     * @ghidraAddress 0x00317100
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyList_Check(pyob);
    }

    /**
     * Report the sentinel rather than the length, as Py::String does.
     *
     * @return The value max_size() reports.
     * @ghidraAddress 0x003170d8
     */
    virtual int capacity() const {
        return max_size();
    }
};

} // namespace Py
