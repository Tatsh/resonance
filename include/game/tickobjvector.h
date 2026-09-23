#pragma once

#include <algorithm>
#include <vector>

#include "mid/tickobj.h"

// Searches and sorted insertion over a vector of TickObj ordered by song position.
//
// Each template below was emitted once per element type and per translation unit that used it, and
// one declaration stands for several addresses. The instantiations share their instruction shapes
// exactly and differ only in the comparator and helper addresses they call. The element type of
// each instantiation is not recorded where the call sites have not settled it.
//
// The comparators are called through a function pointer rather than inlined, as std::upper_bound
// and std::lower_bound do with a comparator passed as a plain function. Each search inlines the
// standard algorithm and calls the comparator out of line.

/**
 * Order two entries by song position.
 *
 * Instantiated for `TickObj<float>` at `0x001b7068` in the Phrase unit, and three times in the
 * TrackData unit.
 *
 * @param first The first entry.
 * @param second The second entry.
 * @return Whether first comes strictly before second.
 * @ghidraAddress 0x001b7068
 * @ghidraAddress 0x001d7f70
 * @ghidraAddress 0x001d7f80
 * @ghidraAddress 0x001d7f90
 * @ghidraAddress 0x001a9860
 */
template <typename T>
bool TickObjLess(TickObj<T> first, TickObj<T> second) {
    return first.mPosition.mTick < second.mPosition.mTick;
}

/**
 * Report whether a song position comes strictly before an entry.
 *
 * @param nTick The song position, in MIDI ticks.
 * @param entry The entry.
 * @return Whether nTick comes strictly before entry.
 * @ghidraAddress 0x001b70e0
 * @ghidraAddress 0x001d8028
 * @ghidraAddress 0x001d8038
 * @ghidraAddress 0x001d8048
 */
template <typename T>
bool TickObjBefore(int nTick, TickObj<T> entry) {
    return nTick < entry.mPosition.mTick;
}

/**
 * Report whether an entry comes strictly before a song position.
 *
 * @param entry The entry.
 * @param nTick The song position, in MIDI ticks.
 * @return Whether entry comes strictly before nTick.
 * @ghidraAddress 0x001d80a0
 * @ghidraAddress 0x001a9a10
 */
template <typename T>
bool TickObjAfter(TickObj<T> entry, int nTick) {
    return entry.mPosition.mTick < nTick;
}

/**
 * Find the first entry a value strictly precedes.
 *
 * @param values The entries, sorted by position.
 * @param value The value to place.
 * @return The upper bound of value.
 * @ghidraAddress 0x001b5b40
 * @ghidraAddress 0x001a91a8
 * @ghidraAddress 0x001d5b98
 * @ghidraAddress 0x001d61d8
 * @ghidraAddress 0x001d6898
 * @ghidraAddress 0x001d6a48
 */
template <typename T>
typename std::vector<TickObj<T> >::iterator UpperBoundByEntry(std::vector<TickObj<T> > &values,
                                                              const TickObj<T> &value) {
    return std::upper_bound(values.begin(), values.end(), value, TickObjLess<T>);
}

/**
 * Find the first entry a song position strictly precedes.
 *
 * @param values The entries, sorted by position.
 * @param nTick The song position, in MIDI ticks.
 * @return The upper bound of nTick.
 * @ghidraAddress 0x001b6278
 * @ghidraAddress 0x001d7050
 * @ghidraAddress 0x001d7108
 * @ghidraAddress 0x001d71c0
 */
template <typename T>
typename std::vector<TickObj<T> >::const_iterator
UpperBoundByTick(const std::vector<TickObj<T> > &values, int nTick) {
    return std::upper_bound(values.begin(), values.end(), nTick, TickObjBefore<T>);
}

/**
 * Find the first entry at or after a song position.
 *
 * @param values The entries, sorted by position.
 * @param nTick The song position, in MIDI ticks.
 * @return The lower bound of nTick.
 * @ghidraAddress 0x001d7278
 */
template <typename T>
typename std::vector<TickObj<T> >::const_iterator
LowerBoundByTick(const std::vector<TickObj<T> > &values, int nTick) {
    return std::lower_bound(values.begin(), values.end(), nTick, TickObjAfter<T>);
}

/**
 * Find the last entry at or before a song position.
 *
 * @param values The entries, sorted by position.
 * @param nTick The song position, in MIDI ticks.
 * @return The entry, or the end when every entry follows nTick.
 * @ghidraAddress 0x001b7290
 * @ghidraAddress 0x001d80e8
 * @ghidraAddress 0x001d8130
 * @ghidraAddress 0x001d8058
 */
template <typename T>
typename std::vector<TickObj<T> >::const_iterator
FindAtOrBefore(const std::vector<TickObj<T> > &values, int nTick) {
    if (values.begin() == values.end()) {
        return values.end();
    }
    const auto upper = UpperBoundByTick(values, nTick);
    if (upper == values.begin()) {
        return values.end();
    }
    return upper - 1;
}

/**
 * Find the first entry at or after a song position.
 *
 * @param values The entries, sorted by position.
 * @param nTick The song position, in MIDI ticks.
 * @return The entry, or the end when every entry precedes nTick.
 * @ghidraAddress 0x001d80b0
 */
template <typename T>
typename std::vector<TickObj<T> >::const_iterator
FindAtOrAfter(const std::vector<TickObj<T> > &values, int nTick) {
    if (values.begin() == values.end()) {
        return values.end();
    }
    return LowerBoundByTick(values, nTick);
}

/**
 * Insert an entry at its sorted place, after every entry at the same position.
 *
 * An entry that does not precede the last one is appended without a search.
 *
 * @param values The entries, sorted by position.
 * @param value The entry to insert.
 * @ghidraAddress 0x001b5e80
 * @ghidraAddress 0x001a90c0
 * @ghidraAddress 0x001d6960
 * @ghidraAddress 0x001d6b10
 */
template <typename T>
void InsertSorted(std::vector<TickObj<T> > &values, const TickObj<T> &value) {
    if (values.size() != 0 && !(value.mPosition.mTick < values.back().mPosition.mTick)) {
        values.push_back(value);
        return;
    }
    values.insert(UpperBoundByEntry(values, value), value);
}

/**
 * Store a value at a song position, overwriting the entry the search lands on when that entry sits
 * at the same position.
 *
 * The search is the upper bound and lands past every entry at the same position. The overwrite
 * branch is reachable only through the comparator's definition of order.
 *
 * @param values The entries, sorted by position.
 * @param value The value.
 * @param nTick The song position, in MIDI ticks.
 * @ghidraAddress 0x001d5f00
 * @ghidraAddress 0x001d6540
 */
template <typename T>
void SetAtTick(std::vector<TickObj<T> > &values, const T &value, int nTick) {
    TickObj<T> entry;
    entry.mPosition.mTick = nTick;
    entry.mValue = value;
    typename std::vector<TickObj<T> >::iterator it = UpperBoundByEntry(values, entry);
    if (it != values.end() && it->mPosition.mTick == nTick) {
        *it = entry;
        return;
    }
    values.insert(it, entry);
}

/**
 * Insert an entry at its upper bound, always searching first.
 *
 * MultiMuse::Add() takes this path when it is told not to try appending.
 *
 * @param values The entries, sorted by position.
 * @param value The entry to insert.
 * @ghidraAddress 0x001a9a20
 */
template <typename T>
void InsertAtUpperBound(std::vector<TickObj<T> > &values, const TickObj<T> &value) {
    values.insert(UpperBoundByEntry(values, value), value);
}

/**
 * Insert a value at a song position, after every entry at the same position.
 *
 * @param values The entries, sorted by position.
 * @param value The value.
 * @param nTick The song position, in MIDI ticks.
 * @ghidraAddress 0x001d7fa0
 * @ghidraAddress 0x001d7fc8
 * @ghidraAddress 0x001d8000
 */
template <typename T>
void InsertAtTick(std::vector<TickObj<T> > &values, const T &value, int nTick) {
    TickObj<T> entry;
    entry.mPosition.mTick = nTick;
    entry.mValue = value;
    InsertSorted(values, entry);
}
