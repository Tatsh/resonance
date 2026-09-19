#pragma once

#include "app/msgsink.h"
#include "msg/message.h"

/**
 * Abstract base of the game's sound output.
 *
 * `5Synth` in the RTTI descriptor at `0x008ef430`, with MsgSink as its one base. No vtable for this
 * class exists in the image, so it is never instantiated. Two classes implement it: Ps2HardSynth,
 * which drives the hardware, and the file-local `Synth::Setup::NullSynth` at `0x008eecf8`, whose
 * table at `0x007d2e50` answers eleven of the interface slots with an empty body and is therefore
 * the null implementation. A third file-local class, `Synth::Setup::SynthFade` at `0x008eebe8`,
 * derives from TimeTask rather than from this class.
 *
 * The interface is seventeen slots, 0 through 16, which NullSynth's terminator fixes. Slot 0 is
 * GetTypeInfo and slot 1 the destructor; slot 2 is MsgSink::Handle() inherited unchanged; slot 3 is
 * the HandleMessage() override below. Slots 4 through 16 are thirteen further virtuals that this
 * class declares. Ps2HardSynth's table has eighteen slots and adds one of its own at slot 17, which
 * is consistent with the two implementations being siblings rather than one deriving from the
 * other.
 *
 * Those thirteen are **not declared here**, because no string in the image identifies them. No
 * assert or log string in either implementation mentions a member of this class, so a declaration
 * would have to invent thirteen titles and thirteen signatures. What is known about each is
 * recorded instead:
 *
 * | Slot | NullSynth | Ps2HardSynth | Shape |
 * | ---- | --------- | ------------ | ----- |
 * | 4 | `0x0013a1d0` empty | `0x003f47b8` | takes an argument, returns a word |
 * | 5 | `0x0013a1d8` empty | `0x003f4960` | takes an argument, returns a word |
 * | 6 | `0x0013a1e0` empty | `0x003f4af0` | returns a word |
 * | 7 | `0x0013a1e8` empty | not overridden | |
 * | 8 | `0x0013a1f0` empty | `0x003f6650` | no argument, no result |
 * | 9 | `0x0013a478` empty | `0x003f66d8` | no argument, no result |
 * | 10 | `0x0013a1f8` empty | not overridden | |
 * | 11 | `0x0013a200` empty | `0x003f65c8` | takes arguments, returns a word |
 * | 12 | `0x0013a208` empty | `0x003f6700` | no argument, no result |
 * | 13 | `0x0013a210` empty | `0x003f6740` | no argument, no result |
 * | 14 | `0x0013a218` empty | `0x003f6720` | no argument, no result |
 * | 15 | `0x0013a220` | `0x003f4c50` | returns a word |
 * | 16 | `0x0013a288` | not overridden | this class supplies the body |
 *
 * The argument counts above come from which registers each body reads and are a lower bound only,
 * so they are recorded as prose rather than turned into parameter lists.
 *
 * The attested behaviour of this subsystem sits in the driver at `0x00462558` through `0x00465000`
 * rather than in either class. That module owns the voices and the sound banks, and its own strings
 * state what it does: `Using %d voices total`, `Voice %2.2d at %x env %x - end %d mix %d %d %d %d`,
 * `BD bank loading returned async error %d`, and `Unrecognized synth cmd %d`. Ps2HardSynth is a
 * thin class over it.
 */
class Synth : public MsgSink {
public:
    /**
     * @ghidraAddress 0x0013a398
     */
    virtual ~Synth();

protected:
    /**
     * Act on a message.
     *
     * This class supplies the body and neither implementation overrides it.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x0013a570
     */
    virtual void HandleMessage(Message *pMsg);
};
