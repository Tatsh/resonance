#pragma once

/**
 * Base of every object that reads a controller directly.
 *
 * `13RawController` in the RTTI descriptor at `0x0086f658`, with no base. Its vtable at
 * `0x007dc698` has three entries and a zero terminator at index 3, the type function at
 * `0x001935e8`, the destructor at `0x00193628`, and one slot filled with the pure-virtual stub at
 * `0x005381a8`. That stub is the same address MsgSink's own table records for its pure
 * HandleMessage(), which is what establishes the third slot as pure rather than as a body of its
 * own.
 *
 * GrooveWorld, MetaGameWorld, and InputCheatDetector all derive from the class, and each overrides
 * the third slot. GrooveWorld places its subobject at `+0x04` and its secondary table at
 * `0x007dc628` adjusts `this` by `-4` on every entry.
 *
 * The class declares no data member. Both of its recovered overrides write only the object they
 * belong to, and the destructor stores the table pointer and nothing else.
 */
class RawController {
public:
    /**
     * @ghidraAddress 0x00193628
     */
    virtual ~RawController();

    /**
     * Report a controller reading. Slot 2.
     *
     * Pure in this class. GrooveWorld fills it at `0x0018ed98` and MetaGameWorld at `0x003d3288`.
     * Both implementations package the four arguments into a RawControllerMsg, whose payload is
     * three words followed by the float in the same order, and dispatch it. The member name is a
     * placeholder rather than a recovered verb.
     *
     * The signature is recovered from the two implementations, which agree. Each reads a1, a2, and
     * a3 and moves f12 into a saved float register before doing anything else. Only the last
     * parameter's type is distinguished by the registers; the three words ahead of it are written
     * as int because nothing in either body narrows them further.
     *
     * @param nUnknown1 The first word of the reading.
     * @param nUnknown2 The second word of the reading.
     * @param nUnknown3 The third word of the reading.
     * @param flUnknown4 The float of the reading.
     */
    virtual void OnUnknownSlot2(int nUnknown1, int nUnknown2, int nUnknown3, float flUnknown4) = 0;
};
