#pragma once

#include "app/msgqueue.h"
#include "app/msgsink.h"

class Message;

/**
 * Base of every renderer, with a message queue and a router of its own.
 *
 * `12RendererBase` in the RTTI descriptor, deriving from MsgSink at offset 0, with its own type
 * function at `0x00139e88`. Three classes derive from it: MetRenderer, MetNullRenderer, and
 * Renderer. Each of their constructors calls this class's constructor directly.
 *
 * The table at `0x007d2d20` runs eleven entries against MsgSink's four, so slots 4 through 10 are
 * the seven virtuals this class introduces. Slots 3, 7, and 8 address the pure-virtual stub at
 * `0x005381a8`, and the class is therefore abstract. Slot 3 is MsgSink::HandleMessage(), which this
 * class leaves pure. Slot 2 overrides MsgSink::Handle().
 *
 * No verb is recovered for any of the seven, so each is declared under a placeholder that records
 * its slot. The six MetRenderer overrides take no argument and return nothing, and Renderer's
 * three overrides agree, which is what fixes the signatures. The base bodies of slots 4, 5, 9, and
 * 10 are empty and read no argument register, so they reveal nothing on their own.
 */
class RendererBase : public MsgSink {
public:
    /**
     * Sink that forwards every message it receives to one other sink.
     *
     * `Q212RendererBase6Router` in the RTTI descriptor at `0x00901fa0`, recorded as single
     * inheritance from MsgSink at offset 0. The object is eight bytes: the four-byte MsgSink
     * subobject, whose table pointer sits at offset 0, followed by the target at `+0x04`.
     *
     * 172 tables in the image place the shared MsgSink::Handle() body at slot 2. This class and
     * RendererBase are the two that override it, and the tables of RendererBase's subclasses
     * inherit RendererBase's override.
     *
     * The destructor at `0x00139d70` is implicitly declared. It is byte-identical to MsgSink's,
     * because a trivial derived destructor stores only the base table pointer.
     */
    class Router : public MsgSink {
    public:
        /**
         * Forward the message to the target sink.
         *
         * The body reads the target from `+0x04`, takes the delta and the function pointer of that
         * target's own table slot 3, and dispatches with the adjusted receiver. The message itself
         * is never touched: it stays in the argument register the caller placed it in and passes
         * through to the target unchanged. Dispatching slot 3 rather than slot 2 means the target's
         * HandleMessage() runs directly, so a chain of routers cannot form.
         *
         * @param pMsg The message to forward.
         * @ghidraAddress 0x00139f50
         */
        virtual void Handle(Message *pMsg);

        /**
         * Receive a message directly, which this class ignores.
         *
         * The body is empty. The class exists to forward through Handle(), so the slot that would
         * consume a message has nothing to do. MsgSink declares it pure, so an override has to
         * exist for the class to be concrete.
         *
         * @param pMsg The message, which the body does not read.
         * @ghidraAddress 0x00139f48
         */
        virtual void HandleMessage(Message *pMsg);

        /**
         * The sink every message is handed to. +0x04
         *
         * Public because RendererBase's constructor writes it directly, and an enclosing class has
         * no access to a nested class's private members. A friend declaration fits equally well.
         */
        MsgSink *mTarget;
    };

    /**
     * Start the queue with the router as its one sink.
     *
     * The router's target is this object, so every message the queue delivers arrives at this
     * object's HandleMessage().
     *
     * @ghidraAddress 0x00139c10
     */
    RendererBase();

    /**
     * Release the queue and the router.
     *
     * The body writes the MsgSink table pointer at `+0x00` and again at `+0x40`, tears the queue
     * down through MsgQueue::~MsgQueue(), and releases the object under its `__in_chrg` flag. The
     * second table write is the router member rather than a second base, because the RTTI records
     * MsgSink as the one base of this class and Router as a nested class with its own descriptor.
     *
     * @ghidraAddress 0x00139e20
     */
    virtual ~RendererBase();

    /**
     * Accept a message into the queue.
     *
     * Slot 2. The message is stored rather than acted on, and reaches HandleMessage() only when
     * OnUnknownSlot6() drains the queue. MetRenderer and Renderer both inherit this body.
     *
     * @param pMsg The message to store.
     * @ghidraAddress 0x00139f80
     */
    virtual void Handle(Message *pMsg);

    /**
     * Unrecovered. Slot 4, with an empty body in this class.
     *
     * @ghidraAddress 0x00139f28
     */
    virtual void OnUnknownSlot4();

    /**
     * Unrecovered. Slot 5, with an empty body in this class.
     *
     * @ghidraAddress 0x00139f30
     */
    virtual void OnUnknownSlot5();

    /**
     * Deliver every queued message. Slot 6, whose verb is unrecovered.
     *
     * The body drains the queue through MsgQueue::Poll(). MetRenderer inherits it, and Renderer's
     * override calls it explicitly.
     *
     * @ghidraAddress 0x00139fb0
     */
    virtual void OnUnknownSlot6();

    /** Unrecovered. Slot 7, pure in this class. */
    virtual void OnUnknownSlot7() = 0;

    /** Unrecovered. Slot 8, pure in this class. */
    virtual void OnUnknownSlot8() = 0;

    /**
     * Unrecovered. Slot 9, with an empty body in this class.
     *
     * @ghidraAddress 0x00139f38
     */
    virtual void OnUnknownSlot9();

    /**
     * Unrecovered. Slot 10, with an empty body in this class.
     *
     * @ghidraAddress 0x00139f40
     */
    virtual void OnUnknownSlot10();

private:
    // Destroyed by the destructor through MsgQueue::~MsgQueue() at 0x0054a7a0, which is what
    // identifies the member. +0x04
    MsgQueue mQueue;
    // The table pointer the destructor writes at +0x40 is this member's MsgSink subobject. +0x40
    Router mRouter;
};
