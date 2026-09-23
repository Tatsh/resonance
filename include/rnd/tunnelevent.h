#pragma once

namespace Rnd {

class Drawable;
class Object;
class Stream;

/**
 * One drawable scheduled at a frame of a Rnd::Tunnel.
 *
 * The record is not polymorphic and has no RTTI, and the image retains no title for it. The name
 * is inferred from the routines of Rnd::Tunnel that keep a list of these ordered by frame.
 *
 * The record is 0x10 bytes, the value of a 0x18-byte list node. The loader at 0x0046de48 settles
 * the type of mObject, because it resolves the stored name through `dynamic_cast` to
 * Rnd::Drawable.
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

    /**
     * Write the event.
     *
     * The drawable is written by name, followed by mFrame, mId, and mUser.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0046dd40
     */
    void Save(Stream &stream) const;

    /**
     * Read the event.
     *
     * mUser is cleared first and read only from a Rnd::Tunnel stream revision of 32 or later. No
     * reference is taken on the drawable.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x0046de48
     */
    void Load(Stream &stream);

    /**
     * Replace the drawable when it is pFrom, moving the reference held on behalf of pReferrer.
     *
     * @param pFrom The object being replaced.
     * @param pTo The replacement, which must be a Rnd::Drawable or null.
     * @param pReferrer The object the reference is held on behalf of.
     * @ghidraAddress 0x00477630
     */
    void Replace(Object *pFrom, Object *pTo, Object *pReferrer);

    Drawable *mObject; /*!< The drawable, referenced by the owning tunnel. +0x00 */
    float mFrame;      /*!< The frame the list is ordered by. +0x04 */
    int mId;           /*!< The identifier the lookups match on. +0x08 */
    int mUser;         /*!< A word no reader was located for. +0x0c */
};

} // namespace Rnd
