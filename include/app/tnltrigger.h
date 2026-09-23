#pragma once

/**
 * An action AppTunnel schedules for a later frame.
 *
 * `10TnlTrigger` in the RTTI descriptor, a class with no base. Its type function is at `0x00458578`
 * and its vtable at `0x0081c000`. The vptr is the whole object, so the class has no data members.
 * Slot 1 is the destructor, and slot 2 is the pure virtual Fire(), which still addresses the
 * pure-virtual stub in the class's own table. TnlPanelFXDelay is the only derived class.
 *
 * AppTunnel keeps each pending trigger with its frame in a TnlPendingTrigger, and deletes the
 * trigger once it has fired.
 */
class TnlTrigger {
public:
    /**
     * Destroy the trigger.
     *
     * The deleting destructor at `0x004585b8` is compiler-generated.
     */
    virtual ~TnlTrigger() {
    }

    /** Perform the scheduled action. */
    virtual void Fire() = 0;
};
