#pragma once

/**
 * Value that moves toward a target at a fixed rate, read through a linear mapping.
 *
 * The class is not polymorphic and emits no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The head-up display and the tunnel both embed it by value.
 *
 * The raw value runs from 0 toward mTarget at mRate per unit of Execute()'s time, and Val()
 * reports it as `mCurrent * mScale + mOffset`. SetParams() fixes the mapping and the rate.
 */
class LinearAnim {
public:
    /**
     * Start at 0 with no time recorded, mapping 0 through 1 onto 0 through 100 over 240 units.
     *
     * @ghidraAddress NTSC-U/C: 0x00411878
     * @ghidraAddress PAL: 0x0044b340
     */
    LinearAnim();

    /**
     * Set the mapping and the rate.
     *
     * A raw value of 0 maps to flFrom and 1 maps to flTo, and the raw value covers that span in
     * flDuration units of time.
     *
     * @param flFrom The mapped value at raw 0.
     * @param flTo The mapped value at raw 1.
     * @param flDuration The time the raw value takes to cover 0 through 1.
     * @ghidraAddress NTSC-U/C: 0x004118d0
     * @ghidraAddress PAL: 0x0044b398
     */
    void SetParams(float flFrom, float flTo, float flDuration);

    /**
     * Set the raw value to move toward.
     *
     * @param flTarget The raw target.
     * @ghidraAddress NTSC-U/C: 0x00411900
     * @ghidraAddress PAL: 0x0044b3c8
     */
    void SetTarget(float flTarget);

    /**
     * Set the target and move the raw value almost onto it at once.
     *
     * The raw value lands one millionth short of the target, computed in double precision. The next
     * Execute() then finishes the move and reports a change. The title is inferred.
     *
     * @param flTarget The raw target.
     * @ghidraAddress NTSC-U/C: 0x00411908
     * @ghidraAddress PAL: 0x0044b3d0
     */
    void Jump(float flTarget);

    /**
     * Report the mapped value.
     *
     * @return `mCurrent * mScale + mOffset`.
     * @ghidraAddress NTSC-U/C: 0x00411958
     * @ghidraAddress PAL: 0x0044b420
     */
    float Val() const;

    /**
     * Move the raw value toward the target by the time elapsed since the last call.
     *
     * The first call after construction only records the time. A step that would overshoot lands
     * on the target.
     *
     * @param flTime The current time.
     * @return 1 when the raw value was away from the target on entry, and 0 when it was already
     *         there.
     * @ghidraAddress NTSC-U/C: 0x00411970
     * @ghidraAddress PAL: 0x0044b438
     */
    int Execute(float flTime);

private:
    // The time Execute() last ran at. 9999999 marks no time recorded.
    float mLastTime; // +0x00
    float mTarget;   // +0x04
    float mCurrent;  // +0x08
    // Raw units per unit of time.
    float mRate;   // +0x0c
    float mScale;  // +0x10
    float mOffset; // +0x14
};
