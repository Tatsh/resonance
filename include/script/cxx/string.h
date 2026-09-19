#pragma once

#include "os/hxstr.h"
#include "script/cxx/char.h"
#include "script/cxx/config.h"
#include "script/cxx/seqbase.h"

namespace Py {

/**
 * Handle on a Python string.
 *
 * `Q22Py6String` in the RTTI descriptor at `0x008f0990`, with `Py::SeqBase<Py::Char>` at offset 0
 * as its one base. Its accessor is at `0x004c65d8`.
 *
 * The vtable at `0x00821cb8` has nine entries, and the type function, the destructor, accepts(),
 * and slot 4 differ from the base table.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x004c65d8` |
 * | 1 | compiler-generated destructor | `0x004c6560` |
 * | 2 | `accepts` | `0x004c73a8` |
 * | 3 | inherited, unidentified | `0x004c7260` |
 * | 4 | unidentified | `0x004c7270` |
 * | 5 | inherited, unidentified | `0x004c7890` |
 * | 6 | inherited, unidentified | `0x004c73d0` |
 * | 7 | inherited `getItem` | `0x004c7a48` |
 * | 8 | inherited `setItem` | `0x004c7bb8` |
 *
 * The conversion to HxStr below is one half of the port's substitution of HxStr for
 * `std::string`. Released PyCXX has the same member returning a `std::string`.
 */
class String : public SeqBase<Char> {
public:
    /**
     * Take a borrowed reference to an existing string.
     *
     * @param pyob The string to wrap.
     * @ghidraAddress 0x004c4b40
     */
    explicit String(PyObject *pyob) : SeqBase<Char>(pyob) {
        validate();
    }

    /**
     * Build a Python string from a game string.
     *
     * Only the signature is recovered, from the call in PyShell::ReportError() that passes the
     * error context to `hxutl.traceback_str`. The body is not worked out, so none is written.
     *
     * @param text The text to copy.
     * @ghidraAddress 0x004c4d88
     */
    explicit String(const HxStr &text);

    /**
     * Accept only a string.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a string.
     * @ghidraAddress 0x004c73a8
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyString_Check(pyob);
    }

    /**
     * Copy the text out of the Python string.
     *
     * @return The text.
     * @ghidraAddress 0x004c73f0
     */
    operator HxStr() const {
        return HxStr(PyString_AsString(mPtr));
    }
};

} // namespace Py
