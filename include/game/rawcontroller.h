#pragma once

/**
 * Base of every object that reads a controller directly.
 *
 * `13RawController` in the RTTI descriptor at `0x0086f658`, with no base. Its vtable is at
 * `0x007dc698` and has three entries, the type function at `0x001935e8`, the destructor at
 * `0x00193628`, and one slot filled with the pure-virtual stub at `0x005381a8`. That stub is the
 * same address MsgSink's own table records for its pure `HandleMessage()`, which is what
 * establishes the third slot as pure rather than as a body of its own.
 *
 * GrooveWorld, MetaGameWorld, and InputCheatDetector all derive from the class, and each overrides
 * the third slot. GrooveWorld places its subobject at `+0x04` and its secondary table at
 * `0x007dc628` adjusts `this` by `-4` on every entry.
 *
 * Recovery has barely started. This declaration exists so that MetaGameWorld and GrooveWorld can
 * name the base their descriptors record.
 */
class RawController {
public:
    /**
     * @ghidraAddress 0x00193628
     */
    virtual ~RawController();

    /**
     * Unrecovered. Slot 2.
     *
     * Pure in this class. GrooveWorld fills it at `0x0018ed98` and MetaGameWorld at `0x003d3288`.
     * Neither the purpose nor the argument list is established, and the empty parameter list is a
     * placeholder rather than a recovered signature.
     */
    virtual void OnUnknownSlot2() = 0;
};
