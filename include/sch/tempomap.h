#pragma once

#include "app/attachment.h"

namespace Sch {

/**
 * Affine map between a song position in MIDI ticks and scheduler time in nanoseconds.
 *
 * `Q23Sch8TempoMap` in the RTTI descriptor at `0x008f2a10`, deriving publicly from Attachment at
 * offset 0. The descriptor address is verified against the accessor at `0x0052d260`, which guards
 * on `0x008f2a10` and then calls TypeInfo::ConstructSingleInheritance() with the mangled name at
 * `0x00827ca0` and Attachment's own descriptor at `0x0086f5a0`.
 *
 * The vtable at `0x00827cb0` runs three entries and then a zero entry, so the class declares no
 * virtual of its own:
 *
 *  - 0, the compiler-generated type function at `0x0052d260`
 *  - 1, the destructor at `0x0052d240`
 *  - 2, Attachment::Destroy() at `0x004bfea8`, inherited
 *
 * An instance is 0x28 bytes. Three call sites allocate that size (`0x001ea8c0`, `0x001ec628`, and
 * `0x004a7a38`), the constructor's highest write lands at `+0x20`, and the four trailing bytes are
 * the padding that the 8-byte alignment of the three wide members forces. The layout therefore
 * closes.
 *
 * Every member is public, because code outside the hierarchy reads each one directly and the image
 * exposes no accessor. Sch::TickClock reads mNanosecondsPerTick and mOriginNanoseconds at
 * `0x004a627c` and `0x004a6280` to convert a song position into scheduler time, reads
 * mNanosecondsPerTick and mCeilingBias at `0x004a7b2c` and `0x004a7b28` to convert the other way,
 * and the `clock tempo` script command reads mMicrosecondsPerQuarter at `0x00151214`.
 *
 * Two facts fix the units, and each is measured twice. The tempo is in microseconds per quarter
 * note, because Sch::TickClock's constructor at `0x004a7a48` supplies the default 500000, which is
 * the tempo a Standard MIDI File assumes, and the `clock tempo` command reports the member back
 * unscaled. The resolution is 480 ticks per quarter note, because the scale is the tempo times
 * 25/12, that is 1000 nanoseconds per microsecond over 480 ticks per quarter note, and because the
 * `song_bar` command at `0x001ac774` divides a song position by 1920, which is four quarter notes
 * at the same resolution. Scheduler time is in nanoseconds, because Watchdog's clock scales its
 * millisecond readings by `1.0e9 / 1000.0` at `0x005124c8`, and because the run loop at
 * `0x004aa8a4` treats a gap of 6000000000 as the point at which it stops catching up.
 */
class TempoMap : public Attachment {
public:
    /**
     * Start a map at the requested tempo, with song position zero at scheduler time zero.
     *
     * @param nMicrosecondsPerQuarter The tempo, in microseconds per quarter note.
     * @ghidraAddress 0x0052d118
     */
    TempoMap(int nMicrosecondsPerQuarter);

    /**
     * Release the map.
     *
     * The body forwards to the Attachment destructor and touches no member of its own.
     *
     * @ghidraAddress 0x0052d240
     */
    virtual ~TempoMap();

    /**
     * Change the tempo from the requested song position onwards.
     *
     * The scheduler time of that song position is preserved across the change, so a tempo change
     * moves every later position without moving any earlier one. The title is inferred from the
     * `clock tempo` script command at `0x0015132c`, which is the one caller.
     *
     * @param nMicrosecondsPerQuarter The new tempo, in microseconds per quarter note.
     * @param nTick The song position the new tempo takes effect at, in MIDI ticks.
     * @ghidraAddress 0x0052d198
     */
    void SetTempo(int nMicrosecondsPerQuarter, long long nTick);

    /** Nanoseconds each MIDI tick lasts at the current tempo. +0x08 */
    long long mNanosecondsPerTick;

    /** Scheduler time, in nanoseconds, that song position zero maps to. +0x10 */
    long long mOriginNanoseconds;

    /**
     * Numerator bias that turns the scheduler-time-to-tick division into a rounding-up one.
     *
     * The value is always mNanosecondsPerTick minus mOriginNanoseconds minus one, which makes
     * `(time + mCeilingBias) / mNanosecondsPerTick` the smallest MIDI tick at or after the
     * requested time. Sch::TickClock::SongTick() at `0x004a7b28` is the one reader.
     *
     * +0x18
     */
    long long mCeilingBias;

    /** The current tempo, in microseconds per quarter note. +0x20 */
    int mMicrosecondsPerQuarter;
};

} // namespace Sch
