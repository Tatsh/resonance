#pragma once

#include <vector>

#include "met/metscreen.h"

/**
 * Auto-repeat state for the front end, one record per controller.
 *
 * The class emits no RTTI and declares no allocation operator, for the same reasons recorded on
 * MetCommandMap, and it shares that translation unit. The name here is inferred from what the class
 * does and is not attested anywhere in the image.
 *
 * The object is twelve bytes and its one member is a `std::vector` of four twenty-four-byte
 * records, one per controller from pad 1 to pad 4. A held navigation command arms its controller's
 * record, which then waits an initial three seconds before re-delivering the command at the
 * interval the active panel's MetScreen::mUnknown58 sets.
 */
class MetCommandRepeater {
public:
    /**
     * Build four empty records.
     *
     * @ghidraAddress 0x002e54b0
     */
    MetCommandRepeater();

    /**
     * Release the vector.
     *
     * @ghidraAddress 0x002e71c0
     */
    ~MetCommandRepeater();

    /**
     * Disarm every record.
     *
     * Writes zero to the command of all four records and nothing else, which is a four-iteration
     * loop with no bound read from the vector. The count is therefore the literal four rather than
     * the vector's size.
     *
     * @ghidraAddress 0x002e7298
     */
    void Reset();

    /**
     * Arm one record from a command that has just been delivered.
     *
     * The record of pad nPadIndex is rearmed when the command is not 0 or when the record already
     * belongs to nButton. It then takes the button and the command, starts the three-second
     * initial delay, and records the current time. MetRenderer::HandleMessage() is the one caller,
     * and it runs the routine only for a `joy ` reading whose command is one of 0, 1, 2, 3, 4,
     * 0x10, 0x11, 0x12, or 0x13.
     *
     * @param pCommand The command that was delivered.
     * @param nButton The reading's mButton.
     * @param nPadIndex The reading's mPadIndex, counted from one.
     * @ghidraAddress 0x002e5790
     */
    void Arm(const MetScreenCommand *pCommand, int nButton, int nPadIndex);

    /**
     * Re-deliver every armed command whose interval has elapsed.
     *
     * The interval is MetScreen::mUnknown58 multiplied by 50, in milliseconds, read from the panel
     * the routine is given rather than stored per record. A record whose initial delay is still
     * positive has the milliseconds since it was last serviced subtracted from it instead of
     * firing. A null panel is rejected at once.
     *
     * @param pPanel The screen the repeated commands are delivered to.
     * @param pNowNanoseconds The current time, which the caller has already computed.
     * @ghidraAddress 0x002e55b0
     */
    void Update(MetScreen *pPanel, const long long *pNowNanoseconds);

private:
    // One controller's repeat state. Twenty-four bytes.
    struct Record {
        // Only the delay and the time are initialised, which is what the constructor's prototype
        // record writes.
        Record() : mDelayMs(0), mLastNs(0) {
        }

        int mDelayMs;      // +0x00 the initial delay still to run
        int mButton;       // +0x04
        int mCommand;      // +0x08 zero while disarmed
        int mUnknown0c;    // +0x0c padding before the eight-byte time
        long long mLastNs; // +0x10 when the record was last serviced
    };

    std::vector<Record> mRecords; // +0x00
};
