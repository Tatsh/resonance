#pragma once

#include <vector>

#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/runtimeerror.h"

namespace Py {

/**
 * Build a method record for MethodTable::add().
 *
 * A free function rather than a member, and inline, so it has no address of its own. Its shape
 * comes from MethodTable::add(), which assembles the four words on its own stack in this order
 * before handing them to the vector.
 *
 * @param pszMethodName The Python-visible name.
 * @param pfnMethod The C entry point.
 * @param nFlags The calling convention flag, which every recovered registration passes as 1.
 * @param pszDoc The doc string.
 * @return The record.
 */
inline PyMethodDef method(const char *pszMethodName,
                          PyCFunction pfnMethod,
                          int nFlags = 1,
                          const char *pszDoc = nullptr) {
    PyMethodDef record;
    record.ml_name = const_cast<char *>(pszMethodName);
    record.ml_meth = pfnMethod;
    record.ml_flags = nFlags;
    record.ml_doc = const_cast<char *>(pszDoc);
    return record;
}

/**
 * Accumulator for the method table of an extension module.
 *
 * `Q22Py11MethodTable` in the RTTI descriptor at `0x0086f688`, with no base. The object is 0x14
 * bytes, the vector at `+0x00`, the flattened table at `+0x0c`, and the vptr after both at
 * `+0x10`, which is where this compiler places the vptr of a class with no base.
 *
 * The vtable at `0x00833210` has two entries.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x005ab9d0` |
 * | 1 | `~MethodTable` | `0x005abb68` |
 *
 * The one instance in the image is the function-local static behind ScriptFunc's constructor,
 * which the game uses to register every script function exported to Python.
 */
class MethodTable {
public:
    /**
     * Start with the terminator record alone.
     *
     * @ghidraAddress 0x005a5aa8
     */
    MethodTable() {
        mRecords.push_back(method(nullptr, nullptr, 0, nullptr));
        mTable = nullptr;
    }

    /**
     * Release the flattened table.
     *
     * @ghidraAddress 0x005abb68
     */
    virtual ~MethodTable() {
        delete[] mTable;
    }

    /**
     * Append one method ahead of the terminator.
     *
     * Appending after table() has run is refused, because the flattened copy is what the
     * interpreter already holds.
     *
     * @param pszMethodName The Python-visible name.
     * @param pfnMethod The C entry point.
     * @param pszDoc The doc string.
     * @param nFlags The calling convention flag.
     * @ghidraAddress 0x005a5bc8
     */
    void
    add(const char *pszMethodName, PyCFunction pfnMethod, const char *pszDoc = "", int nFlags = 1) {
        if (mTable != nullptr) {
            throw RuntimeError(HxStr("Too late to add a module method!"));
        }
        mRecords.insert(mRecords.end() - 1, method(pszMethodName, pfnMethod, nFlags, pszDoc));
    }

    /**
     * Flatten the records into an array the interpreter can retain.
     *
     * The array is built once and reused, and that is also what closes the table to further
     * additions.
     *
     * @return The array, terminated by the record the constructor placed.
     * @ghidraAddress 0x005abc98
     */
    PyMethodDef *table() {
        if (mTable == nullptr) {
            int nCount = static_cast<int>(mRecords.size());
            mTable = new PyMethodDef[nCount];
            for (int i = 0; i < nCount; ++i) {
                mTable[i] = mRecords[i];
            }
        }
        return mTable;
    }

private:
    std::vector<PyMethodDef> mRecords; // +0x00
    PyMethodDef *mTable;               // +0x0c
};

} // namespace Py
