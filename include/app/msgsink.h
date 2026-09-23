#pragma once

#include <cstddef>

class Message;

/**
 * Receiver of engine messages.
 *
 * `7MsgSink` in the RTTI descriptor at `0x0086f780`, built from the length-prefixed literal at
 * `0x007cccb0` with no base list. The class declares no data member, and the compiler-generated
 * vptr therefore lands at offset 0 over a four-byte subobject. Player corroborates the size by
 * placing its MsgSink base at `+0x08` and its MsgSource base at `+0x0c`. 134 classes derive from
 * MsgSink and 40 of them derive directly.
 *
 * The vtable at `0x007ccc40` runs four entries and a zero terminator: the type function, the
 * destructor, Handle(), and HandleMessage(). The HandleMessage() entry addresses the shared
 * pure-virtual stub at `0x005381a8`, the target of 355 slots across the image. That entry marks
 * the member pure rather than defaulted.
 *
 * The destructor and Handle() are defined inline. g++ 2.9x emitted the vtable into every
 * translation unit that constructs or destroys a derived object, producing 45 byte-identical
 * copies, and emitted the two bodies alongside them. 48 further copies of the destructor, 104 of
 * Handle(), and 44 of the type function remain in the image. Every copy that occupies no vtable
 * slot is referenced only from a frame-unwind record. The reconstruction therefore owes two
 * definitions rather than 196.
 *
 * Five derived classes have an implicitly declared destructor emitted at a distinct address and
 * byte-identical to this one. A trivial derived destructor stores only the base table pointer once
 * the compiler discards the dead store of its own. Those five are NoteFinder at `0x00105588`,
 * RendererBase::Router at `0x00139d70`, MidiChase at `0x001a67d8`, the file-local Shifter of
 * GsMuseUtil.cpp at `0x001ab878`, and RiffRangeFinder at `0x001c4200`. The reconstruction declares
 * none of the five. The compiler generates each one.
 */
class MsgSink {
public:
    /**
     * Allocate a sink from the tagged heap under the tag "MsgSink".
     *
     * No out-of-line body exists. Every allocation of a derived class inlines the call, 78 sites
     * in all, among them the Synth unit's file-local NullSynth::New() at `0x0013a0c0` and
     * `new MetArenasScreen` at `0x001fc690`. Both pass the literal `MsgSink`. MsgSource declares
     * no allocation pair. Its destructor at `0x0054a168` proves that by releasing its vector
     * without a tagged free, and a class deriving from both bases therefore resolves the operator
     * here without ambiguity.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    void *operator new(size_t nSize);

    /**
     * Release a sink to the tagged heap.
     *
     * No out-of-line body exists. The release branch of every derived destructor inlines the call
     * with the literal `MsgSink`, 294 sites in all.
     *
     * @param pBlock The block.
     */
    void operator delete(void *pBlock);

    /**
     * @ghidraAddress 0x00105120
     */
    virtual ~MsgSink();

    /**
     * Accept a message.
     *
     * The body dispatches table slot 3, HandleMessage(), through the object's own vptr. Two
     * overrides are recovered. RendererBase::Router at `0x00139f50` forwards the message to the
     * sink it stores, and RendererBase at `0x00139f80` stores the message in its queue. Every other
     * MsgSink subobject table in the image places this body at slot 2.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00105158
     */
    virtual void Handle(Message *pMsg);

    /**
     * Act on a message.
     *
     * Public rather than protected. The counter-example is one of the two classes that override
     * Handle(). RendererBase::Router at `0x00139f50` dispatches this member's slot on the separate
     * sink it stores at `+0x04`, which is an object of an unrelated class, so protected access
     * would not reach it. Every other dispatch in the image does come from Handle() on the same
     * object, which is why the narrower reading held until that override was found. A friend
     * declaration fits equally well.
     *
     * @param pMsg The message.
     */
    virtual void HandleMessage(Message *pMsg) = 0;
};
