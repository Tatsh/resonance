#pragma once

/**
 * Receiver of the values an ActiveFilter produces.
 *
 * `11FilterLover`, whose type name is at `0x007cc648` in the `AppActiveFilter.cpp` unit. The class
 * is an interface with no data. AxeFX derives from it at `+0x18`, and the FilterLover table inside
 * AxeFX's (at `0x007dd7d8`) has the destructor at slot 1 and OnFilterValue() at slot 2.
 */
class FilterLover {
public:
    /**
     * Release nothing. Slot 1.
     *
     * The body is empty. The out-of-line emission frees the object through the scalar release on
     * its deleting path.
     *
     * @ghidraAddress 0x0019b278
     */
    virtual ~FilterLover() {
    }

    /**
     * Receive the filter's new output.
     *
     * Slot 2. ActiveFilter::Update() runs it once per step. The title is inferred.
     *
     * @param flValue The smoothed value.
     */
    virtual void OnFilterValue(float flValue) = 0;
};
