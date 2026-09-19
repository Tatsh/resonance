#pragma once

#include "met/metscreen.h"

/**
 * Auto-repeat state for the front end, one record per controller.
 *
 * The class emits no RTTI and declares no allocation operator, for the same reasons recorded on
 * MetCommandMap, and it shares that translation unit. The name here is inferred from what the class
 * does and is not attested anywhere in the image.
 *
 * The object is twelve bytes and its one member is a `std::vector` of twenty-four-byte records,
 * which the destructor at `0x002e71c0` fixes by dividing the byte span by twenty-four.
 * MetRenderer's constructor sizes the vector at four, and MetRenderer::mUnknownd4 accepts pad
 * indices up to four on the same evidence. Each record stores a countdown at `+0x00`, a value the
 * command's mButton is filled from at `+0x04`, the pending command code at `+0x08`, and the
 * sixty-four-bit time the record was last serviced at `+0x10`. Reset() clears `+0x08` of all four,
 * which is what makes that field the armed flag. The three remaining words of the record are not
 * recovered, so the member is recorded here rather than declared.
 */
class MetCommandRepeater {
public:
    /**
     * Build four empty records.
     *
     * The body is not written. It zeroes the vector and resizes it to four copies of a zeroed
     * prototype.
     *
     * @ghidraAddress 0x002e54b0
     */
    MetCommandRepeater();

    /**
     * Release the vector.
     *
     * The body is not written.
     *
     * @ghidraAddress 0x002e71c0
     */
    ~MetCommandRepeater();

    /**
     * Disarm every record.
     *
     * Writes zero to `+0x08` of all four records and nothing else, which is a four-iteration loop
     * over a stride of twenty-four with no bound read from the vector. The count is therefore the
     * literal four rather than the vector's size, and a vector of any other length would not
     * agree.
     *
     * @ghidraAddress 0x002e7298
     */
    void Reset();

    /**
     * Arm one record from a command that has just been delivered.
     *
     * The body is not written. MetRenderer::HandleMessage() is the one caller, and it runs the
     * routine only for a `joy ` reading whose command is one of 0, 1, 2, 3, 4, 0x10, 0x11, 0x12,
     * or 0x13.
     *
     * @param pCommand The command that was delivered.
     * @param nButton The reading's mButton.
     * @param nPadIndex The reading's mPadIndex.
     * @ghidraAddress 0x002e5790
     */
    void Arm(const MetScreenCommand *pCommand, int nButton, int nPadIndex);

    /**
     * Re-deliver every armed command whose interval has elapsed.
     *
     * The body is not written. The interval is MetScreen::mUnknown58 multiplied by 50, in
     * milliseconds, read from the panel the routine is given rather than stored per record. A
     * record whose countdown at `+0x00` is positive has the elapsed milliseconds subtracted from it
     * instead of firing, which is the initial delay before the repeat starts. A null panel is
     * rejected at once.
     *
     * @param pPanel The screen the repeated commands are delivered to.
     * @param pNowNanoseconds The current time, which the caller has already computed.
     * @ghidraAddress 0x002e55b0
     */
    void Update(MetScreen *pPanel, const long long *pNowNanoseconds);
};
