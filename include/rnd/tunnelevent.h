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
 *
 * An inline member that draws the event through a filter has an out-of-line copy at 0x004776f8 and
 * no caller. While mUser is set and a filter object is passed, it runs slot 2 of the filter with
 * its second argument and mUser and draws mObject through Rnd::Drawable::Draw() only when the slot
 * reports non-zero. Otherwise it draws mObject at once. The filter's class is not recovered,
 * because no call site passes one, so the member is not declared.
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
     * @param nUser The key a filtered draw passes to its filter.
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
    int mUser;         /*!< The key the filtered draw at 0x004776f8 passes to its filter. +0x0c */
};

} // namespace Rnd
