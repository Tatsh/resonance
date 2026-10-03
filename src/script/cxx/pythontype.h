#pragma once

#include "script/cxx/config.h"

namespace Py {

/**
 * Builder for the CPython type object of an extension type.
 *
 * Its RTTI descriptor is at `0x0086f588`. It has no base. Its accessor is at `0x005aba50`, and the
 * mangled name string sits at `0x00833400`.
 *
 * The object is 0x18 bytes, the five table pointers at `+0x00` to `+0x10` and the vptr after them
 * at `+0x14`. The vtable at `0x008331e0` has two entries, the type function at `0x005aba50` and
 * the destructor at `0x005ac220`.
 *
 * The constructor takes no type name, unlike released PyCXX's, and every type it builds is called
 * `unknown`. The `support` setters that fill the protocol tables are outside this declaration.
 */
class PythonType {
public:
    /**
     * Allocate the type object and clear every slot but the size, the name, and the deallocator.
     *
     * @param nBasicSize The instance size.
     * @param nItemSize The size of one variable-length item.
     * @ghidraAddress NTSC-U/C: 0x005ac120
     * @ghidraAddress PAL: 0x005ee648
     */
    PythonType(int nBasicSize, int nItemSize);

    /**
     * Free the type object and the protocol tables.
     *
     * @ghidraAddress NTSC-U/C: 0x005ac220
     * @ghidraAddress PAL: 0x005ee748
     */
    virtual ~PythonType();

    /**
     * Read the type object.
     *
     * @return The type object.
     * @ghidraAddress NTSC-U/C: 0x005ac298
     * @ghidraAddress PAL: 0x005ee7c0
     */
    PyTypeObject *type_object() const {
        return mTable;
    }

    /**
     * Set the deallocator.
     *
     * @param pfnDealloc The deallocator.
     * @ghidraAddress NTSC-U/C: 0x005ac2c0
     * @ghidraAddress PAL: 0x005ee7e8
     */
    void dealloc(destructor pfnDealloc) {
        mTable->tp_dealloc = pfnDealloc;
    }

    /**
     * Give the type the sequence protocol, once.
     *
     * The first call allocates the table, attaches it, and points its first seven entries at the
     * Py::PythonExtensionBase handlers. The three entries after them remain unwritten.
     *
     * @ghidraAddress NTSC-U/C: 0x005abf90
     * @ghidraAddress PAL: 0x005ee4b8
     */
    void supportSequenceType();

    /**
     * Give the type the mapping protocol, once.
     *
     * @ghidraAddress NTSC-U/C: 0x005ac040
     * @ghidraAddress PAL: 0x005ee568
     */
    void supportMappingType();

    /**
     * Give the type the buffer protocol, once.
     *
     * The character buffer entry remains unwritten.
     *
     * @ghidraAddress NTSC-U/C: 0x005ac0b0
     * @ghidraAddress PAL: 0x005ee5d8
     */
    void supportBufferType();

private:
    // The deallocator a type has until dealloc() replaces it.
    static void standard_dealloc(PyObject *pyob);

    PyTypeObject *mTable;
    PySequenceMethods *mSequenceTable;
    PyMappingMethods *mMappingTable;
    PyNumberMethods *mNumberTable;
    PyBufferProcs *mBufferTable;
};

} // namespace Py
