#pragma once

#include "app/msgsink.h"
#include "msg/message.h"

/**
 * Enclosing class of Router, declared only so that the nesting can be expressed.
 *
 * `12RendererBase` in the RTTI descriptor, with its own type function at `0x00139e88` and its
 * destructor at `0x00139e20`. The class itself is not recovered. Its table has entries at
 * `0x00139f28`, `0x00139f30`, `0x00139f38`, and `0x00139f40`, none of which is titled, so nothing
 * here declares them; a later pass owns that work.
 *
 * This declaration exists because Router is a nested class and C++ cannot express the nesting
 * without it. Adding a member to this class on anything short of recovered evidence would be
 * worse than the gap.
 */
class RendererBase {
public:
    /**
     * Sink that forwards every message it receives to one other sink.
     *
     * `Q212RendererBase6Router` in the RTTI descriptor at `0x00901fa0`, recorded as single
     * inheritance from MsgSink at offset 0. The object is eight bytes: the four-byte MsgSink
     * subobject, whose table pointer sits at offset 0, followed by the target at `+0x04`.
     *
     * The class is the one counter-example to a claim MsgSink's own header used to make. 172
     * tables in the image place the shared MsgSink::Handle() body at slot 2, and this class is the
     * only one that overrides it.
     */
    class Router : public MsgSink {
    public:
        /** @ghidraAddress 0x00139d70 */
        virtual ~Router();

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
         * Public because no accessor for it exists in the image and nothing recovered writes it
         * either, so whatever installs the target is outside the part of the program recovered so
         * far. A friend declaration fits equally well.
         */
        MsgSink *mTarget;
    };
};
