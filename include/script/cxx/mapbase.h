#pragma once

#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/exception.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on any Python mapping, and the base of Py::Dict.
 *
 * One instantiation exists in the image, `Q22Pyt7MapBase1ZQ22Py6Object`, the descriptor at
 * `0x009029a0`, deriving from Py::Object at offset 0. The harvest does not demangle the name,
 * because its demangler does not handle the template form, so the name comes from the mangled
 * field instead.
 *
 * The vtable at `0x00825848`, copied at `0x00833278`, has five entries, the three Py::Object slots
 * plus two of its own.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x0050d0b0` |
 * | 1 | inherited destructor | `0x0050d038` |
 * | 2 | `accepts` | `0x0050d7f8` |
 * | 3 | `setItem` taking a name | `0x0050d6f8` |
 * | 4 | `setItem` taking a key object | `0x0050d780` |
 *
 * Both setItem() bodies are shared with Py::Dict rather than overridden, so the derived table
 * repeats the same two addresses.
 *
 * Released PyCXX declares a reading half as well, with `size()`, `getItem()`, and a mapping
 * reference proxy. None of it is in the table, so none is reconstructed. The image therefore
 * establishes that this instantiation is write-only.
 */
template <typename T>
class MapBase : public Object {
protected:
    /**
     * Produce a handle on `None` for a derived class that fills it in afterwards.
     *
     * The body is empty, which is what the folded vptr write in Py::Dict's default constructor
     * establishes. This compiler omits the intermediate vptr store when the intermediate
     * constructor runs no code between the two stores.
     */
    MapBase() {
    }

    /**
     * Take a borrowed reference on behalf of a derived class.
     *
     * @param pyob The reference to wrap.
     */
    explicit MapBase(PyObject *pyob) : Object(pyob) {
        validate();
    }

public:
    /**
     * Accept any reference the mapping protocol supports.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a mapping.
     * @ghidraAddress 0x0050d7f8
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyMapping_Check(pyob) != 0;
    }

    /**
     * Store a value under a name.
     *
     * @param key The key, as text. An empty string arrives at the interpreter as
     *            g_szEmptyString rather than as a null pointer.
     * @param value The value to store.
     * @ghidraAddress 0x0050d6f8
     */
    virtual void setItem(const HxStr &key, const Object &value) {
        char *pszKey = const_cast<char *>(key.mStr != nullptr ? key.mStr : g_szEmptyString);
        if (PyMapping_SetItemString(mPtr, pszKey, value.mPtr) == -1) {
            throw Exception();
        }
    }

    /**
     * Store a value under a key object.
     *
     * @param key The key.
     * @param value The value to store.
     * @ghidraAddress 0x0050d780
     */
    virtual void setItem(const Object &key, const Object &value) {
        if (PyObject_SetItem(mPtr, key.mPtr, value.mPtr) == -1) {
            throw Exception();
        }
    }
};

} // namespace Py
