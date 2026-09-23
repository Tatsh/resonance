#pragma once

/**
 * Value that moves toward a target at a fixed rate, read through a linear mapping.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, allocation tag, or file path
 * identifies it, and its name is inferred from its behaviour. The head-up display and the tunnel
 * both embed it by value.
 *
 * The raw value runs from 0 toward mTarget at mRate per unit of Update()'s time, and Value()
 * reports it as `mCurrent * mScale + mOffset`. SetRange() fixes the mapping and the rate.
 */
class LinearRamp {
public:
    /**
     * Start at 0 with no time recorded, mapping 0 through 1 onto 0 through 100 over 240 units.
     *
     * @ghidraAddress 0x00411878
     */
    LinearRamp();

    /**
     * Set the mapping and the rate.
     *
     * A raw value of 0 maps to flFrom and 1 maps to flTo, and the raw value covers that span in
     * flDuration units of time.
     *
     * @param flFrom The mapped value at raw 0.
     * @param flTo The mapped value at raw 1.
     * @param flDuration The time the raw value takes to cover 0 through 1.
     * @ghidraAddress 0x004118d0
     */
    void SetRange(float flFrom, float flTo, float flDuration);

    /**
     * Set the raw value to move toward.
     *
     * @param flTarget The raw target.
     * @ghidraAddress 0x00411900
     */
    void SetTarget(float flTarget);

    /**
     * Set the target and move the raw value almost onto it at once.
     *
     * The raw value lands one millionth short of the target, computed in double precision. The next
     * Update() then finishes the move and reports a change. The title is inferred.
     *
     * @param flTarget The raw target.
     * @ghidraAddress 0x00411908
     */
    void Jump(float flTarget);

    /**
     * Report the mapped value.
     *
     * @return `mCurrent * mScale + mOffset`.
     * @ghidraAddress 0x00411958
     */
    float Value();

    /**
     * Move the raw value toward the target by the time elapsed since the last call.
     *
     * The first call after construction only records the time. A step that would overshoot lands
     * on the target.
     *
     * @param flTime The current time.
     * @return 1 when the raw value was away from the target on entry, and 0 when it was already
     *         there.
     * @ghidraAddress 0x00411970
     */
    int Update(float flTime);

private:
    // The time Update() last ran at. 9999999 marks no time recorded.
    float mLastTime; // +0x00
    float mTarget;   // +0x04
    float mCurrent;  // +0x08
    // Raw units per unit of time.
    float mRate;   // +0x0c
    float mScale;  // +0x10
    float mOffset; // +0x14
};
