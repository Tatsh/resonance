#pragma once

#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/exception.h"
#include "script/cxx/fromapi.h"
#include "script/cxx/typeerror.h"

namespace Py {

// Py::String derives from Py::SeqBase<Py::Char>, which derives from this class, so the two
// headers cannot include each other. str() returns a String by value and is defined out of line
// in src/script/cxx/object.cpp, where the complete type is available.
class String;

/**
 * Reference-counted handle on any Python object, and the root of the PyCXX object hierarchy.
 *
 * `Q22Py6Object` in the RTTI descriptor at `0x0086f6f8`, with no base. The object is eight bytes,
 * the reference at `+0x00` and the vptr after it at `+0x04`, which is where this compiler places
 * the vptr of a class with no base.
 *
 * The vtable at `0x007cdd00` has three entries, and several translation units emit a private copy
 * of the same three.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x0010ee30` |
 * | 1 | `~Object` | `0x0010ee70` |
 * | 2 | `accepts` | `0x0010eee8` |
 *
 * Every derived class in the hierarchy inherits those three slots and appends its own, so a slot
 * index above 2 always belongs to a derived class.
 *
 * Two differences from released PyCXX are established rather than assumed. The port has no
 * `owned` constructor flag, and the constructor below always adds a reference; an owned reference
 * arrives through Py::FromAPI instead. The port also replaced `std::string` with HxStr throughout.
 */
class Object {
public:
    /**
     * Take a borrowed reference and add a count of its own.
     *
     * The default produces a handle on `None`, which is what a member of this type gets when its
     * owner supplies no initialiser.
     *
     * @param pyob The reference to wrap.
     */
    explicit Object(PyObject *pyob = Py_None) : mPtr(pyob) {
        Py_XINCREF(mPtr);
        validate();
    }

    /**
     * Copy another handle and add a count of its own.
     *
     * No out-of-line copy survives, because this compiler builds every returned Object directly
     * in the caller's slot. The body comes instead from the inlined copy inside
     * PyShell::ReportError() at `0x005082e0`, where a Py::Callable is built from a temporary
     * Py::Object and the three steps are all visible.
     *
     * @param other The handle to copy.
     */
    Object(const Object &other) : mPtr(other.mPtr) {
        Py_XINCREF(mPtr);
        validate();
    }

    /**
     * Release the reference.
     *
     * @ghidraAddress 0x0010ee70
     */
    virtual ~Object() {
        release();
    }

    /**
     * Replace the reference with another handle's.
     *
     * @param other The handle to copy.
     * @return This handle.
     */
    Object &operator=(const Object &other) {
        set(other.mPtr);
        return *this;
    }

    /**
     * Whether a reference is acceptable for this handle's type.
     *
     * The base test accepts any non-null reference, so a bare Object wraps anything. A derived
     * class narrows it, and validate() enforces the narrowed test.
     *
     * @param pyob The reference to test.
     * @return True when the reference suits this type.
     * @ghidraAddress 0x0010eee8
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr;
    }

    /**
     * Read one attribute.
     *
     * Only the signature is recovered, from the call in PyShell::ReportError() that fetches
     * `traceback_str` out of the `hxutl` module. The body is not worked out, so none is written.
     *
     * @param name The attribute name.
     * @return A handle on the attribute.
     * @ghidraAddress 0x004c3f80
     */
    Object getAttr(const HxStr &name) const;

    /**
     * Produce the text of `str()` applied to the object.
     *
     * @return A new handle on the string object.
     * @ghidraAddress 0x0055c980
     */
    String str() const;

    /**
     * Produce the text of `str()` applied to the object, as a game string.
     *
     * @return The text.
     * @ghidraAddress 0x0055cb30
     */
    HxStr as_string() const;

    /**
     * Report whether the object is a tuple.
     *
     * Inline. The type word is compared with `PyTuple_Type` without a null test, which is how
     * MetHelpScreen::FillTexts() at `0x00313208` expands it.
     *
     * @return True for a tuple.
     */
    bool isTuple() const {
        return PyTuple_Check(mPtr);
    }

    /**
     * Report whether the object is a list.
     *
     * Inline, expanded the same way as isTuple().
     *
     * @return True for a list.
     */
    bool isList() const {
        return PyList_Check(mPtr);
    }

    /**
     * Wrapped reference, null only between release() and the next assignment.
     *
     * Public because new_reference_to() and several call sites across the script layer read it
     * directly and the image has no accessor for it. A trivial accessor and a public member
     * compile to the same single load, so the evidence cannot distinguish them, and the public
     * member adds no function that no address can be attached to.
     *
     * +0x00
     */
    PyObject *mPtr;

protected:
    /**
     * Release the reference and empty the handle.
     *
     * No address of its own survives. The body appears inlined in the destructor, and released
     * PyCXX declares the member, so it is reconstructed here as the shared form.
     */
    void release() {
        Py_XDECREF(mPtr);
        mPtr = nullptr;
    }

    /**
     * Replace the reference with a borrowed one and add a count of its own.
     *
     * The old reference is released before the new one is stored, and the handle is never left
     * empty in between, which is why the body does not use release().
     *
     * @param pyob The reference to wrap.
     * @ghidraAddress 0x004c6a30
     */
    void set(PyObject *pyob) {
        Py_XDECREF(mPtr);
        mPtr = pyob;
        Py_XINCREF(mPtr);
        validate();
    }

    /**
     * Enforce this type's accepts() test on the stored reference.
     *
     * A reference the type rejects is released first. A Python error already posted is then
     * rethrown as a bare Py::Exception, because the interpreter already holds the detail;
     * otherwise a Py::TypeError posts `CXX: type error.` and is thrown instead.
     *
     * The message form settles the build configuration of the binding. Released PyCXX writes the
     * failing type name into the message when run-time type information is available and falls
     * back on this fixed text otherwise, and the image has the fixed text, so the binding was
     * built with neither `_CPPRTTI` nor `__GNUG__` reaching that block.
     *
     * @ghidraAddress 0x004c3d10
     */
    void validate() {
        if (accepts(mPtr)) {
            return;
        }
        release();
        if (PyErr_Occurred() != nullptr) {
            throw Exception();
        }
        throw TypeError(HxStr("CXX: type error."));
    }
};

/**
 * Produce an owned reference from a handle, for returning to the interpreter.
 *
 * @param object The handle to read.
 * @return The reference with one count added, which the caller then owns.
 * @ghidraAddress 0x004c6f28
 */
inline PyObject *new_reference_to(const Object &object) {
    PyObject *pyob = object.mPtr;
    Py_XINCREF(pyob);
    return pyob;
}

} // namespace Py
