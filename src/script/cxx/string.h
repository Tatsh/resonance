#pragma once

#include "os/hxstr.h"
#include "script/cxx/char.h"
#include "script/cxx/config.h"
#include "script/cxx/seqbase.h"

namespace Py {

/**
 * Handle on a Python string.
 *
 * Its RTTI descriptor is at `0x008f0990`. It has `Py::SeqBase<Py::Char>` at offset 0 as its one
 * base. Its accessor is at `0x004c65d8`.
 *
 * The vtable at `0x00821cb8` has nine entries, and the type function, the destructor, accepts(),
 * and slot 4 differ from the base table.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x004c65d8` |
 * | 1 | compiler-generated destructor | `0x004c6560` |
 * | 2 | `accepts` | `0x004c73a8` |
 * | 3 | inherited `max_size` | `0x004c7260` |
 * | 4 | `capacity` | `0x004c7270` |
 * | 5 | inherited `swap` | `0x004c7890` |
 * | 6 | inherited `size` | `0x004c73d0` |
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
     * @ghidraAddress NTSC-U/C: 0x004c4b40
     * @ghidraAddress PAL: 0x00502dc8
     */
    explicit String(PyObject *pyob) : SeqBase<Char>(pyob) {
        validate();
    }

    /**
     * Build a Python string from a game string.
     *
     * The base wraps the result of `PyString_FromString()` at `0x0059d6b8`, stored in a
     * Py::FromAPI temporary, and the body runs validate(). A null game string converts as the
     * empty string. PyShell::ReportError() passes the error context to `hxutl.traceback_str` this
     * way.
     *
     * @param text The text to copy.
     * @ghidraAddress NTSC-U/C: 0x004c4d88
     * @ghidraAddress PAL: 0x00503010
     */
    explicit String(const HxStr &text);

    /**
     * Build a Python string from a C string.
     *
     * The base wraps the result of `PyString_FromString()` at `0x0059d6b8`, stored in a
     * Py::FromAPI temporary, and the body runs validate(). The pointer arrives at the interpreter
     * unchanged.
     *
     * @param pszText The text to copy.
     * @ghidraAddress NTSC-U/C: 0x004c5138
     * @ghidraAddress PAL: 0x005033c0
     */
    explicit String(const char *pszText);

    /**
     * Take another handle's reference as a string.
     *
     * The out-of-line body copies the reference with a count of its own and runs validate() under
     * the Py::Object, `SeqBase<Char>`, and Py::String vptrs in turn. It belongs to the vendored
     * binding and is not reconstructed.
     *
     * @param ob The handle to copy.
     * @ghidraAddress NTSC-U/C: 0x004c4c60
     * @ghidraAddress PAL: 0x00502ee8
     */
    explicit String(const Object &ob);

    /**
     * Accept only a string.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a string.
     * @ghidraAddress NTSC-U/C: 0x004c73a8
     * @ghidraAddress PAL: 0x005055d0
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyString_Check(pyob);
    }

    /**
     * Report the sentinel rather than the length.
     *
     * The base returns size(), and this override tail-calls slot 3 instead, so the capacity of a
     * Python string reads as the not-found sentinel. A Python string is immutable, so a capacity
     * equal to its length would invite a caller to write into it.
     *
     * @return The value max_size() reports.
     * @ghidraAddress NTSC-U/C: 0x004c7270
     * @ghidraAddress PAL: 0x00505498
     */
    virtual int capacity() const {
        return max_size();
    }

    /**
     * Copy the text out of the Python string.
     *
     * @return The text.
     * @ghidraAddress NTSC-U/C: 0x004c73f0
     * @ghidraAddress PAL: 0x00505618
     */
    operator HxStr() const {
        return HxStr(PyString_AsString(mPtr));
    }
};

} // namespace Py
