#pragma once

#include "script/cxx/config.h"
#include "script/cxx/exception.h"
#include "script/cxx/object.h"
#include "script/cxx/seqref.h"

namespace Py {

/**
 * Handle on any Python sequence, and the base of Py::List, Py::Tuple, and Py::String.
 *
 * Two instantiations exist in the image. `Py::SeqBase<Py::Object>` has its descriptor at
 * `0x00902990`, and `Py::SeqBase<Py::Char>` has its descriptor at `0x00902000`. Both derive from
 * Py::Object at offset 0.
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
     * MetNullRenderer::OnRawController() at `0x0030e4c0` expands it the same way, with the
     * `0x007d0ff0` vptr.
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
     * @ghidraAddress NTSC-U/C: 0x0012b328
     * @ghidraAddress PAL: 0x0012ba60
     * @ghidraAddress NTSC-U/C: 0x004c7c30
     * @ghidraAddress PAL: 0x00505e58
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
     * The bodies are explicit specialisations that read the sentinel from `os/hxstr.h`.
     *
     * @return The sentinel.
     * @ghidraAddress NTSC-U/C: 0x0012ad48
     * @ghidraAddress PAL: 0x0012b480
     * @ghidraAddress NTSC-U/C: 0x004c7260
     * @ghidraAddress PAL: 0x00505488
     */
    virtual int max_size() const;

    /**
     * Elements this handle could address without reallocating, which is its length.
     *
     * The routine tail-calls slot 6 through the table, so the value tracks size() in a derived
     * class that narrows either one. Py::String overrides it to return max_size() instead.
     *
     * @return The length.
     * @ghidraAddress NTSC-U/C: 0x0012b378
     * @ghidraAddress PAL: 0x0012bab0
     * @ghidraAddress NTSC-U/C: 0x004c7c80
     * @ghidraAddress PAL: 0x00505ea8
     */
    virtual int capacity() const {
        return size();
    }

    /**
     * Exchange references with another handle of the same kind.
     *
     * Copies the argument into a temporary handle, gives the argument this handle's reference
     * unless the two already match, and then takes the temporary's reference through set(). Each
     * set() runs validate(). A reference the receiving type rejects throws.
     *
     * @param other The handle to exchange with.
     * @ghidraAddress NTSC-U/C: 0x0012b3a0
     * @ghidraAddress PAL: 0x0012bad8
     * @ghidraAddress NTSC-U/C: 0x004c7890
     * @ghidraAddress PAL: 0x00505ab8
     */
    virtual void swap(SeqBase<T> &other);

    /**
     * Number of elements.
     *
     * The two instantiations do not share a body, and that is the one finding here worth pausing
     * on. `SeqBase<Object>` calls `PySequence_Length()` at `0x004a53f0` and `SeqBase<Char>` calls
     * `PyString_Size()` at `0x005a1ca0`, so a single template body cannot produce both. Whether
     * the port wrote an explicit specialisation or routed the call through the element type is
     * not recovered. The bodies are explicit specialisations.
     *
     * @return The length.
     * @ghidraAddress NTSC-U/C: 0x0012b358
     * @ghidraAddress PAL: 0x0012ba90
     * @ghidraAddress NTSC-U/C: 0x004c73d0
     * @ghidraAddress PAL: 0x005055f8
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
     * @ghidraAddress NTSC-U/C: 0x0012ac48
     * @ghidraAddress PAL: 0x0012b380
     * @ghidraAddress NTSC-U/C: 0x004c7a48
     * @ghidraAddress PAL: 0x00505c70
     */
    virtual T getItem(int i) const {
        return T(FromAPI(PySequence_GetItem(mPtr, i)).mPtr);
    }

    /**
     * Write one element.
     *
     * @param i The index.
     * @param value The element to store.
     * @ghidraAddress NTSC-U/C: 0x0012b4d0
     * @ghidraAddress PAL: 0x0012bc08
     * @ghidraAddress NTSC-U/C: 0x004c7bb8
     * @ghidraAddress PAL: 0x00505de0
     */
    virtual void setItem(int i, const T &value) {
        if (PySequence_SetItem(mPtr, i, value.mPtr) == -1) {
            throw Exception();
        }
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

    /**
     * Position within a sequence, as PyCXX's `SeqBase<T>::iterator` is laid out.
     *
     * The object is the pair of the sequence's address and an index. PlayMapLinear::LoadStepRings()
     * at `0x00128410` expands every member it uses. Equality compares the two sequence addresses
     * and the two indices, not the Python objects, and end() reads PySequence_Length() afresh each
     * time the loop tests against it. Dereferencing builds a seqref<T>, which fetches the element
     * through getItem(), the virtual at `+0x38`.
     */
    class iterator {
    public:
        /**
         * Address one position.
         *
         * @param pSequence The sequence.
         * @param nWhere The index.
         */
        iterator(SeqBase<T> *pSequence, int nWhere) : mSequence(pSequence), mCount(nWhere) {
        }

        /**
         * Report whether two iterators address the same position of the same sequence.
         *
         * @param other The iterator to compare with.
         * @return True when both the sequence and the index match.
         */
        bool operator==(const iterator &other) const {
            return mSequence == other.mSequence && mCount == other.mCount;
        }

        /**
         * Report whether two iterators address different positions.
         *
         * @param other The iterator to compare with.
         * @return True when the sequence or the index differs.
         */
        bool operator!=(const iterator &other) const {
            return mSequence != other.mSequence || mCount != other.mCount;
        }

        /**
         * Address the element at this position.
         *
         * @return A proxy for the element.
         */
        seqref<T> operator*() {
            return seqref<T>(*mSequence, mCount);
        }

        /**
         * Step to the next position.
         *
         * @return This iterator.
         */
        iterator &operator++() {
            ++mCount;
            return *this;
        }

    private:
        SeqBase<T> *mSequence;
        int mCount;
    };

    /**
     * Address the first element.
     *
     * @return An iterator at index 0.
     */
    iterator begin() {
        return iterator(this, 0);
    }

    /**
     * Address one past the last element.
     *
     * @return An iterator at length().
     */
    iterator end() {
        return iterator(this, length());
    }
};

/** The sequence handle over plain objects, as released PyCXX spells it. */
typedef SeqBase<Object> Sequence;

} // namespace Py
