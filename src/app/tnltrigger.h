#pragma once

/**
 * An action AppTunnel schedules for a later frame.
 *
 * It is a class with no base. Its type function is at `0x00458578` and its vtable at `0x0081c000`.
 * The vptr is the whole object. Slot 1 is the destructor, and slot 2 is the pure virtual Fire().
 * Slot 2 of the class's table addresses the pure-virtual stub. TnlPanelFXDelay is the only derived
 * class.
 *
 * AppTunnel keeps each pending trigger with its frame in a TnlPendingTrigger, and deletes the
 * trigger once it has fired.
 */
class TnlTrigger {
public:
    /**
     * Destroy the trigger.
     *
     * The compiler emits the deleting form out of line.
     *
     * @ghidraAddress 0x004585b8
     */
    virtual ~TnlTrigger() {
    }

    /**
     * Perform the scheduled action.
     *
     * @param flFrame The frame TnlPendingTrigger::Update() was given.
     */
    virtual void Fire(float flFrame) = 0;
};
