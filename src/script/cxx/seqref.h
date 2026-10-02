#pragma once

#include "script/cxx/object.h"

namespace Py {

template <typename T>
class SeqBase;

/**
 * Proxy for one element of a sequence, which SeqBase::operator[]() returns.
 *
 * The class is inline throughout and emits no RTTI descriptor. The stack frame of
 * MetNullRenderer::OnRawController() at `0x0030e4c0` lays it out as the sequence reference, the
 * index, and a copy of the element, which the constructor fetches through SeqBase::getItem(). The
 * name is released PyCXX's.
 */
template <typename T>
class seqref {
public:
    /**
     * Address one element and fetch it.
     *
     * @param sequence The sequence.
     * @param nOffset The index.
     */
    seqref(SeqBase<T> &sequence, int nOffset)
        : mSequence(sequence), mOffset(nOffset), mItem(sequence.getItem(nOffset)) {
    }

    /**
     * Store an element through the proxy.
     *
     * The proxy's copy takes the value through Py::Object's assignment before the sequence
     * stores it, as PythonExtension::getattr_methods() at `0x005ad5e0` expands it.
     *
     * @param value The element to store.
     * @return This proxy.
     */
    seqref &operator=(const Object &value) {
        mItem = value;
        mSequence.setItem(mOffset, value);
        return *this;
    }

    /**
     * Copy the fetched element out.
     *
     * @return A handle on the element.
     */
    operator T() const {
        return mItem;
    }

    /**
     * Report whether the fetched element is a tuple.
     *
     * @return True for a tuple.
     */
    bool isTuple() const {
        return mItem.isTuple();
    }

private:
    SeqBase<T> &mSequence;
    int mOffset;
    T mItem;
};

} // namespace Py
