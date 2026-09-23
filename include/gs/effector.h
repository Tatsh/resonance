#pragma once

#include "app/msgsource.h"

/**
 * Which effect an Effector applies.
 *
 * Every value is measured twice. The factory at `0x001a0df0` switches on the value through a jump
 * table at `0x007de650` whose seven entries cover 5 through 11, and Type() reports the same value
 * back for each class the factory builds.
 */
enum EffectorType {
    kEffectorTypeVolume = 5,          /*!< Builds a VolumeEffector. */
    kEffectorTypeWah = 6,             /*!< Builds a WahEffector. */
    kEffectorTypeStutter = 7,         /*!< Builds a StutterEffector. */
    kEffectorTypeMidiOnOffFirst = 8,  /*!< A MidiOnOffEffector on controller 0x52. */
    kEffectorTypeMidiOnOffSecond = 9, /*!< A MidiOnOffEffector on controller 0x53. */
    kEffectorTypeMidiOnOffThird = 10, /*!< A MidiOnOffEffector on controller 0x51. */
    kEffectorTypeGhostNotes = 11      /*!< Builds a GhostNotesEffector. */
};

/**
 * One switchable audio effect that a powerup applies to a player's track.
 *
 * `8Effector` in the RTTI descriptor at `0x008eff60`, with MsgSource as its one base. The class
 * declares no data member. An instance is therefore the 0x14 bytes of the base, and the vptr stays
 * where MsgSource put it at `+0x10`. Two measurements fix the size. GhostNotesEffector adds no
 * field and the factory allocates 0x14 bytes for it, and VolumeEffector places its first field at
 * `+0x14`.
 *
 * The table at `0x007de948` runs GetTypeInfo, the destructor, MsgSource::AddSink() and
 * MsgSource::RemoveSink() inherited unchanged, then the two virtuals below. Slot 4 addresses the
 * pure-virtual report routine. That is the evidence for Type() being pure.
 *
 * An effect is expressed as MIDI rather than as a filter. Every derived class that does any work
 * sends a control change through MsgSource::Send(), and the synthesiser downstream applies it. The
 * channel and the controller number are fixed when the effect is built.
 *
 * Five classes derive from this one. VolumeEffector, MidiOnOffEffector, and GhostNotesEffector
 * derive from it alone. WahEffector and StutterEffector derive from TickTask as well, placing their
 * Effector subobject at `+0x20`. Both of those vary their control value over time.
 *
 * WahEffector and StutterEffector are recovered and not written here. TickTask has no declaration
 * in this tree yet, and its constructor at `0x0013ad88` lies outside this subsystem. WahEffector is
 * 0x48 bytes with an unsigned char MIDI channel at `+0x34`, an int depth at `+0x38`, an oscillator
 * pointer at `+0x3c`, the enabled flag at `+0x40`, and a pending flag at `+0x44`. Its tick routine
 * at `0x001a0a10` multiplies the depth by the oscillator's current value and sends controller 0x4a,
 * and its Enable() at `0x001a08f8` sends controller 0x51 with 0x7f or with zero. StutterEffector is
 * also 0x48 bytes, and its second tuning value arrives as a two-element property vector. The
 * factory rejects that vector with `Need 2 stutter parameters` at any other length.
 *
 * The factory at `0x001a0df0` is declared below as CreateForType() with its body not written.
 */
class Effector : public MsgSource {
public:
    /**
     * @ghidraAddress 0x001a18c8
     */
    Effector();

    /**
     * @ghidraAddress 0x001a24a8
     */
    virtual ~Effector();

    /**
     * Report which effect this object applies.
     *
     * @return An EffectorType.
     */
    virtual int Type() = 0;

    /**
     * Switch the effect on or off.
     *
     * The default implementation does nothing, and every derived class that sends MIDI overrides
     * it. A derived class compares the request against its stored flag first and sends nothing when
     * the request repeats the current position.
     *
     * @param bEnabled Non-zero to apply the effect.
     * @ghidraAddress 0x001a2558
     */
    virtual void Enable(int bEnabled);

    /**
     * Build the effect one type selects.
     *
     * The routine allocates the class the type selects, reads that class's tuning values from
     * configuration codes 0x38f through 0x392 (the time-varying effects substitute nTrack into the
     * lookup), and returns the Effector subobject. The returned object's Type() is called once and
     * the result is discarded. JamEffectsMgr's constructor is the recovered caller. The body is not
     * written.
     *
     * @param nType An EffectorType. Any other value leaves the result null, and the Type() call
     *              then dereferences it.
     * @param nChannel The MIDI channel the effect sends on.
     * @param nTrack The track, substituted into the property lookups.
     * @return The new effect.
     * @ghidraAddress 0x001a0df0
     */
    static Effector *CreateForType(int nType, unsigned char nChannel, int nTrack);
};
