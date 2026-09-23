#pragma once

class TnlTrigger;

/**
 * A trigger AppTunnel holds until its frame.
 *
 * The record has no RTTI, and the name is inferred from its one routine. It is 8 bytes, the element
 * of the vector at AppTunnel `+0x60`. AppTunnel appends one per scheduled trigger and erases it
 * once Update() reports that the trigger has fired.
 */
struct TnlPendingTrigger {
    /**
     * Fire and delete the trigger once flFrame reaches mFrame.
     *
     * @param flFrame The current frame.
     * @return Zero once the trigger has fired and been deleted, non-zero while it still waits.
     * @ghidraAddress 0x004572c8
     */
    int Update(float flFrame);

    TnlTrigger *mTrigger; /*!< The trigger, owned by the record until it fires. */
    float mFrame;         /*!< The frame the trigger fires on. */
};
