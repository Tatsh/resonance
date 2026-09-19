#pragma once

#include <iostream.h>

#include "game/freqappearancedetail.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Appearance of a player's avatar.
 *
 * `14FreqAppearance` in the RTTI descriptor at `0x0086f580`, with no base, so the compiler places
 * the vptr after the data at `+0x10` and the class is 0x14 bytes. Its vtable at `0x007d98a0` has
 * four entries and a zero terminator at index 4, the type function, the destructor, and the two
 * transfer members.
 *
 * The purpose of two of the three members is recovered from the diagnostic literals Print() writes.
 * `username=` precedes the string at `+0x00`, so that member is the player username, and
 * ` SkillStatus=` precedes the word at `+0x0c`. The detail object at `+0x08` sits behind ` Freq=`.
 * Both literals are display labels rather than identifiers, and they disagree with each other on
 * capitalisation, so neither is treated as an attested member name and both members retain their
 * recovered-purpose-pending spelling.
 *
 * The layout comes from the copy constructor and the destructor together. The copy constructor
 * takes 0xb0 bytes from the allocator for the detail object and stores the pointer at `+0x08`, and
 * the destructor releases the detail object and the string buffer. Every member is private, because
 * the only readers outside the class are the two transfer members and Print().
 *
 * PSJoinRequestPacket::Save() and PlayerInfo::Save() both delegate an embedded FreqAppearance to
 * slot 2 of this table, and their Load() counterparts to slot 3, which is what establishes the two
 * slots as the transfer pair rather than inferring the roles from this class alone.
 */
class FreqAppearance {
public:
    /**
     * Copy another appearance.
     *
     * The three members are initialised first and the assignment below then replaces the string
     * and the detail object. The initial username and the cleared skill status therefore both
     * survive into the copy only for as long as the assignment takes, and because the assignment
     * does not touch the skill status, a copy always reports a skill status of zero.
     *
     * @param other The appearance to copy.
     * @ghidraAddress 0x00174668
     */
    FreqAppearance(const FreqAppearance &other);

    /**
     * Release the detail object.
     *
     * @ghidraAddress 0x00174730
     */
    virtual ~FreqAppearance();

    /**
     * Write the appearance to a stream.
     *
     * Slot 2. A version word of 8 precedes the payload.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00171060
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the appearance back from a stream.
     *
     * Slot 3. The version word Save() writes is read and discarded, so no version older than the
     * current one is handled differently.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x001747a8
     */
    virtual void Load(IBStream &stream);

    /**
     * Write the username and the skill status to a diagnostic stream.
     *
     * The two literals ` Freq=` and ` SkillStatus=` arrive back to back with no value between them,
     * so the detail object is never written.
     *
     * The member is not virtual and occupies no table slot. PlayerInfo::Print() runs it on its
     * embedded appearance.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00174878
     */
    void Print(ostream &stream);

    /**
     * Replace the username and the detail object with another appearance's.
     *
     * Self-assignment is tested first and does nothing. The skill status is not copied, and the
     * routine returns nothing.
     *
     * @param other The appearance to copy.
     * @ghidraAddress 0x001748e0
     */
    void operator=(const FreqAppearance &other);

public:
    /**
     * Player username, starting as the literal `initial name`.
     *
     * Public rather than private, because MetLoadFreqBaseScreen::UpdateNameLabel() copy-constructs
     * an HxStr straight from `+0x00` of the embedded appearance and the image has no accessor to
     * route that read through. A friend declaration fits the image equally well. +0x00
     */
    HxStr mUnknown00;

private:
    FreqAppearanceDetail *mDetail; // +0x08 owned, 0xb0 bytes
    int mUnknown0c;                // +0x0c the skill status
};
