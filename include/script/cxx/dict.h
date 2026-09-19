#pragma once

#include "script/cxx/config.h"
#include "script/cxx/fromapi.h"
#include "script/cxx/mapbase.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on a Python dictionary.
 *
 * `Q22Py4Dict` in the RTTI descriptor at `0x00902190`, with `Py::MapBase<Py::Object>` at offset 0
 * as its one base. Its accessor is at `0x0050d1a0`.
 *
 * The vtable at `0x00825818`, copied at `0x00833248`, has five entries, and only the type
 * function, the destructor, and accepts() differ from the base table. Both setItem() slots repeat
 * the base addresses.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x0050d1a0` |
 * | 1 | compiler-generated destructor | `0x0050d128` |
 * | 2 | `accepts` | `0x0050d248` |
 * | 3 | inherited `setItem` taking a name | `0x0050d6f8` |
 * | 4 | inherited `setItem` taking a key object | `0x0050d780` |
 *
 * Because the base instantiation supplies no reader, neither does this class.
 */
class Dict : public MapBase<Object> {
public:
    /**
     * Create an empty dictionary.
     *
     * The temporary owns the new reference from the interpreter while set() adds a count of its
     * own, and the temporary releases the extra count before validate() runs. PyShell's member
     * takes this path, so the body is recovered from `0x005072e8` rather than from a routine of
     * its own.
     */
    Dict() {
        set(FromAPI(PyDict_New()).mPtr);
        validate();
    }

    /**
     * Take a borrowed reference to an existing dictionary.
     *
     * Recovered from PyShell's constructor, where the reference comes from
     * `PyModule_GetDict()`. The body is inlined at that site and has no address of its own.
     *
     * @param pyob The dictionary to wrap.
     */
    explicit Dict(PyObject *pyob) : MapBase<Object>(pyob) {
        validate();
    }

    /**
     * Accept only a dictionary.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a dictionary.
     * @ghidraAddress 0x0050d248
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyDict_Check(pyob);
    }
};

} // namespace Py
