#pragma once

#include "script/cxx/config.h"
#include "script/cxx/exception.h"
#include "script/cxx/object.h"
#include "script/cxx/seqref.h"

namespace Py {

/**
 * Handle on any Python sequence, and the base of Py::List, Py::Tuple, and Py::String.
 *
 * Two instantiations exist in the image. `Q22Pyt7SeqBase1ZQ22Py6Object` is the descriptor at
 * `0x00902990`, and `Q22Pyt7SeqBase1ZQ22Py4Char` is the descriptor at `0x00902000`. Both derive
 * from Py::Object at offset 0. The harvest demangles neither, because its demangler does not
 * handle the template form, so both names come from the mangled field instead.
 *
 * The vtable has nine entries, the three Py::Object slots plus six of its own. The six below are
 * declared in table order, and that order is itself corroboration: released PyCXX declares
 * `max_size`, `capacity`, `swap`, and `size` in exactly this sequence ahead of the element
 * accessors.
 *
 * | Slot | Member | `SeqBase<Object>` | `SeqBase<Char>` |
 * | ---- | ------ | ----------------- | --------------- |
 * | 0 | compiler-generated type function | `0x0012abd0` | `0x004c64e8` |
 * | 1 | inherited destructor | `0x0012ab58` | `0x004c6470` |
 * | 2 | `accepts` | `0x0012b328` | `0x004c7c30` |
 * | 3 | `max_size` | `0x0012ad48` | `0x004c7260` |
 * | 4 | `capacity` | `0x0012b378` | `0x004c7c80` |
 * | 5 | `swap` | `0x0012b3a0` | `0x004c7890` |
 * | 6 | `size` | `0x0012b358` | `0x004c73d0` |
 * | 7 | `getItem` | `0x0012ac48` | `0x004c7a48` |
 * | 8 | `setItem` | `0x0012b4d0` | `0x004c7bb8` |
 *
 * The three vtables for `SeqBase<Object>` at `0x007d0ff0`, `0x00821c68`, and `0x00825878` are
 * identical copies that the linker did not fold. `SeqBase<Char>` sits at `0x00821d08`.
 */
template <typename T>
class SeqBase : public Object {
protected:
    /**
     * Produce a handle on `None` for a derived class that fills it in afterwards.
     *
     * The body is empty, which is what the folded vptr write in a derived default constructor
     * establishes. This compiler omits the intermediate vptr store when the intermediate
     * constructor runs no code between the two stores.
     */
    SeqBase() {
    }

    /**
     * Take a borrowed reference on behalf of a derived class.
     *
     * Recovered from Py::Tuple's constructor at `0x004c5448`, where the three vptr stores and the
     * three validate() calls of the chain are all visible in one routine.
     *
     * @param pyob The reference to wrap.
     */
    explicit SeqBase(PyObject *pyob) : Object(pyob) {
        validate();
    }

public:
    /**
     * Take a new reference to an existing object as a sequence.
     *
     * Inline. The configuration queries at `0x00509b78` and `0x0050a1a0` expand it as Object's
     * copy, Object's validate(), the store of this class's vptr, and this class's validate().
     *
     * @param ob The object to wrap.
     */
    explicit SeqBase(const Object &ob) : Object(ob) {
        validate();
    }

    /**
     * Accept any reference the sequence protocol supports.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a sequence.
     * @ghidraAddress 0x0012b328
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PySequence_Check(pyob) != 0;
    }

    /**
     * Longest sequence this handle could address.
     *
     * The routine is three instructions and loads one word, the shared not-found sentinel at
     * `0x008211bc`, which is `0xffffffff`. That word sits in HxStr's own literal pool, one word
     * past the empty string the string class returns for a null buffer, and four routines outside
     * the binding read it as well, among them RndText::BuildGlyphMesh(). It is HxStr's `npos`,
     * and released PyCXX returns `std::string::npos` from the same member, which is what settles
     * the reading.
     *
     * No body is written, because the constant belongs in `os/hxstr.h` and this subsystem cannot
     * declare it.
     *
     * @return The sentinel.
     * @ghidraAddress 0x0012ad48
     */
    virtual int max_size() const;

    /**
     * Elements this handle could address without reallocating, which is its length.
     *
     * The routine tail-calls slot 6 through the table, so the value tracks size() in a derived
     * class that narrows either one. Py::String overrides it to return max_size() instead.
     *
     * @return The length.
     * @ghidraAddress 0x0012b378
     */
    virtual int capacity() const {
        return size();
    }

    /**
     * Exchange references with another handle of the same kind.
     *
     * Only the signature is recovered. The body opens by copy-constructing a handle of this type
     * from the argument and then cross-assigns the two references, which is the shape released
     * PyCXX has, and the exchange itself is not worked out, so no body is written.
     *
     * @param other The handle to exchange with.
     * @ghidraAddress 0x0012b3a0
     */
    virtual void swap(SeqBase<T> &other);

    /**
     * Number of elements.
     *
     * The two instantiations do not share a body, and that is the one finding here worth pausing
     * on. `SeqBase<Object>` calls `PySequence_Length()` at `0x004a53f0` and `SeqBase<Char>` calls
     * `PyString_Size()` at `0x005a1ca0`, so a single template body cannot produce both. Whether
     * the port wrote an explicit specialisation or routed the call through the element type is
     * not recovered, and no body is written rather than picking one.
     *
     * @return The length.
     * @ghidraAddress 0x0012b358
     */
    virtual int size() const;

    /**
     * Read one element.
     *
     * The new reference from the interpreter is adopted through a Py::FromAPI temporary, so the
     * returned handle owns exactly one count.
     *
     * @param i The index.
     * @return A handle on the element.
     * @ghidraAddress 0x0012ac48
     */
    virtual T getItem(int i) const {
        return T(FromAPI(PySequence_GetItem(mPtr, i)).mPtr);
    }

    /**
     * Write one element.
     *
     * @param i The index.
     * @param value The element to store.
     * @ghidraAddress 0x0012b4d0
     */
    virtual void setItem(int i, const T &value) {
        if (PySequence_SetItem(mPtr, i, value.mPtr) == -1) {
            throw Exception();
        }
    }

    /**
     * Take another handle's reference as a sequence.
     *
     * Inline. MetNullRenderer::OnRawController() at `0x0030e4c0` expands it as the Py::Object copy,
     * its validate(), the `0x007d0ff0` vptr store, and a second validate().
     *
     * @param ob The handle to copy.
     */
    explicit SeqBase(const Object &ob) : Object(ob) {
        validate();
    }

    /**
     * Number of elements, read without the table.
     *
     * Inline. Unlike size(), the call goes straight to `PySequence_Length()` at `0x004a53f0`, which
     * is how MetHelpScreen::FillTexts() at `0x00313208` expands it.
     *
     * @return The length.
     */
    int length() const {
        return PySequence_Length(mPtr);
    }

    /**
     * Address one element.
     *
     * Inline. The proxy fetches the element through getItem() as it is built.
     *
     * @param i The index.
     * @return A proxy for the element.
     */
    seqref<T> operator[](int i) {
        return seqref<T>(*this, i);
    }
};

/** The sequence handle over plain objects, as released PyCXX spells it. */
typedef SeqBase<Object> Sequence;

} // namespace Py
