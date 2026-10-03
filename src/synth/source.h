#pragma once

#include "app/attachment.h"

/**
 * Reference-counted control signal sampled against elapsed time.
 *
 * Its RTTI descriptor is at `0x008ef3b0`. It has Attachment as its only base at offset 0. The
 * object is Attachment's eight bytes and adds GetValue() as slot 3 of the table it shares with
 * Attachment. The six subclass tables all fill slot 3 and no factory builds a plain Source, and the
 * slot is modelled as pure on that evidence.
 *
 * `source.cpp` defines six subclasses in its anonymous namespace, which the RTTI records under the
 * `_GLOBAL_$N$source.cpp` prefix: Sine, Square, Tri, Ramp, Fade, and HoldAndFadeDown. The static
 * factories below are the only way the image builds any of them. Each allocates through the plain
 * allocator and returns the object with one reference. The implicit destructors of the six
 * subclasses and their type_info accessors are compiler-generated.
 */
class Source : public Attachment {
public:
    /**
     * Produce the signal's value at a time.
     *
     * Slot 3.
     *
     * @param flTime The time since the source started, in the unit its factory arguments use.
     * @param pValue Receives the value, between 0 and 1.
     * @return 1 while the source continues, and 0 once a fade built to stop has finished.
     */
    virtual int GetValue(float flTime, float *pValue) = 0;

    /**
     * Build a sine wave between 0 and 1.
     *
     * @param flPeriod The period.
     * @param flPhase The starting phase, as a fraction of a whole turn.
     * @return The source.
     * @ghidraAddress NTSC-U/C: 0x00545e28
     * @ghidraAddress PAL: 0x00586358
     */
    static Source *AllocateSineSource(float flPeriod, float flPhase);

    /**
     * Build a square wave.
     *
     * The value is 1 while the time modulo the period is below 0.5 and 0 otherwise, whatever the
     * period.
     *
     * @param flPeriod The period.
     * @return The source.
     * @ghidraAddress NTSC-U/C: 0x00545e98
     * @ghidraAddress PAL: 0x005863c8
     */
    static Source *AllocateSquareSource(float flPeriod);

    /**
     * Build a triangle wave that rises from 0 to 1 and falls back once per period.
     *
     * @param flPeriod The period.
     * @param flPhase The starting phase, as a fraction of the period.
     * @return The source.
     * @ghidraAddress NTSC-U/C: 0x00545ee0
     * @ghidraAddress PAL: 0x00586410
     */
    static Source *AllocateTriSource(float flPeriod, float flPhase);

    /**
     * Build a sawtooth that rises from 0 to 1 once per period.
     *
     * @param flPeriod The period.
     * @param flPhase The starting phase, as a fraction of the period.
     * @return The source.
     * @ghidraAddress NTSC-U/C: 0x00545f50
     * @ghidraAddress PAL: 0x00586480
     */
    static Source *AllocateRampSource(float flPeriod, float flPhase);

    /**
     * Build a fade from 1 down to 0 that then holds 0.
     *
     * The title is inferred. The Fade class is shared with AllocateFadeInSource().
     *
     * @param bStopAtEnd Non-zero to report the source finished once the fade is over.
     * @param flDuration The length of the fade.
     * @return The source.
     * @ghidraAddress NTSC-U/C: 0x00545fc0
     * @ghidraAddress PAL: 0x005864f0
     */
    static Source *AllocateFadeOutSource(int bStopAtEnd, float flDuration);

    /**
     * Build a fade from 0 up to 1 that then holds 1.
     *
     * The title is inferred.
     *
     * @param bStopAtEnd Non-zero to report the source finished once the fade is over.
     * @param flDuration The length of the fade.
     * @return The source.
     * @ghidraAddress NTSC-U/C: 0x00546020
     * @ghidraAddress PAL: 0x00586550
     */
    static Source *AllocateFadeInSource(int bStopAtEnd, float flDuration);

    /**
     * Build a signal that holds 1, then fades down to 0 and holds 0.
     *
     * @param bStopAtEnd Non-zero to report the source finished once the fade is over.
     * @param flHold The time the signal holds 1.
     * @param flFade The length of the fade after the hold.
     * @return The source.
     * @ghidraAddress NTSC-U/C: 0x00546088
     * @ghidraAddress PAL: 0x005865b8
     */
    static Source *AllocateHoldAndFadeDownSource(int bStopAtEnd, float flHold, float flFade);
};
