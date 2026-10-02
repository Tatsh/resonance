#pragma once

/**
 * Animation of the arena screens that TnlArena drives.
 *
 * `10ScreenAnim` in the RTTI descriptor at `0x0086f728`, a root class with no base. Its type
 * function is at `0x0040c4e0` and its table at `0x00817308` has five entries. The object is the
 * four-byte vptr alone. TnlArena builds this class itself when configuration code 0x3a1 is set, so
 * that every hook does nothing, and builds SoloScreenAnim or MultiScreenAnim otherwise.
 *
 * The out-of-line copies of the members sit in the TnlArena unit. The hook titles are inferred
 * from the two derived classes and from TnlArena's calls.
 */
class ScreenAnim {
public:
    /** @ghidraAddress 0x0040c520 */
    virtual ~ScreenAnim();

    /**
     * Advance the screens to one song position.
     *
     * Slot 2. The base implementation does nothing.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x0040c550
     */
    virtual void SetFrame(float flFrame);

    /**
     * Select how the screens show the session's state.
     *
     * Slot 3. The base implementation does nothing. TnlArena passes 1 at construction and at
     * destruction.
     *
     * @param nLevel The level, from 0 to 2.
     * @ghidraAddress 0x0040c558
     */
    virtual void SetLevel(int nLevel);

    /**
     * Recompute the players the screens show after a score change.
     *
     * Slot 4. The base implementation does nothing. TnlArena runs it for each PointAmountMsg.
     *
     * @ghidraAddress 0x0040c560
     */
    virtual void UpdateLeaders();
};
