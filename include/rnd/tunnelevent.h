#pragma once

namespace Rnd {

class Drawable;

/**
 * One drawable scheduled at a frame of a Rnd::Tunnel.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * is inferred from the routines of Rnd::Tunnel that keep a list of these ordered by frame.
 *
 * The record is 0x10 bytes, the value of a 0x18-byte list node. The loader at 0x0046de48 settles
 * the type of mObject, because it resolves the stored name through `dynamic_cast` to
 * Rnd::Drawable. The same loader reads mUser only from a stream revision of 0x20 or later.
 */
struct TunnelEvent {
    /**
     * Construct an event with every member unset.
     *
     * The loader grows the list through `resize()` before it fills each value, which needs this
     * constructor.
     */
    TunnelEvent() {
    }

    /**
     * Construct an event.
     *
     * Inline, with one out-of-line emission.
     *
     * @param pObject The drawable.
     * @param flFrame The frame the event is ordered by.
     * @param nId The identifier the lookups match on.
     * @param nUser A word the tunnel stores and passes back unread.
     * @ghidraAddress 0x00477618
     */
    TunnelEvent(Drawable *pObject, float flFrame, int nId, int nUser)
        : mObject(pObject), mFrame(flFrame), mId(nId), mUser(nUser) {
    }

    Drawable *mObject; /*!< The drawable, referenced by the owning tunnel. +0x00 */
    float mFrame;      /*!< The frame the list is ordered by. +0x04 */
    int mId;           /*!< The identifier the lookups match on. +0x08 */
    int mUser;         /*!< A word no reader was located for. +0x0c */
};

} // namespace Rnd
