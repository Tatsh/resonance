#pragma once

#include <iostream>

#include "game/freqappearancedetail.h"
#include "game/freqpart.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Rnd {
class Tex;
}

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
    /** The longest username a Record stores, without its terminator. */
    static constexpr int kRecordNameLength = 12;

    /**
     * Compact form of an appearance that Pack() fills and Unpack() reads.
     *
     * The record has no RTTI and no name in the image, so the title is inferred. Its length
     * follows from the part count, and no routine in the image fixes the largest count. mParts is
     * declared with one entry, and a record runs past it by the remaining parts.
     */
    struct Record {
        bool mValid;                       /*!< Set by Pack(). Unpack() ignores a clear record. */
        char mName[kRecordNameLength + 1]; /*!< The username, cut to 12 characters. */
        unsigned char mSkillStatus;        /*!< The skill status. */
        int mPartCount;                    /*!< The number of entries in mParts. */
        FreqPart::Packed mParts[1];        /*!< The packed parts, mPartCount of them. */
    };

    /**
     * Construct the appearance a new player starts with.
     *
     * The username starts as the literal `initial name`, a fresh 0xb0-byte detail object is
     * allocated, and the skill status is cleared. PSJoinRequestPacket, SPJoinAcceptPacket, and
     * PlayerInfo reach it through their own constructors.
     *
     * @ghidraAddress 0x001745b8
     */
    FreqAppearance();

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
    void Print(std::ostream &stream);

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

    /**
     * Hang this appearance's avatar view in one of the four persona burn slots and show it.
     *
     * Runs InitBurnSlots() first and records the detail object as the slot's occupant. The slot's
     * hangpoint view (`<slot>_freq_hangpoint.view`) then loses its draws and transforms, takes the
     * avatar view as its one child for both, recomposes the avatar's world transform under it, and
     * is shown with everything beneath it. The slot's camera
     * (`persona_texburn_<slot + 1>.cam`) is shown last. MetPersonaData::AttachToBurnSlot() and the
     * routines at `0x003455a8` and `0x0035abf8` call it, and the name is inferred.
     *
     * @param nSlot The burn slot, 0 through 3.
     * @ghidraAddress 0x00171138
     */
    void AttachToBurnSlot(int nSlot);

    /**
     * Resolve the four burn cameras and hangpoint views once.
     *
     * Does nothing after the first call. Otherwise it polls the FreQ maker asset load once, sizes
     * both lists to four, resolves each camera and hides it, and resolves each hangpoint view and
     * shows it. The name is inferred.
     *
     * @ghidraAddress 0x00171398
     */
    static void InitBurnSlots();

    /**
     * Render each shown burn camera into its slot's persona burn texture.
     *
     * Does nothing before InitBurnSlots() has run. For each slot whose camera is shown, the camera
     * recomposes its world transform and draws twice around a GS path sync, and the camera's
     * render target is read back and quantised into the slot's burn texture with one ramp for
     * each part colour of the avatar hung in that slot. The burn texture's palette is then reset,
     * and the camera is hidden. MetRenderer::DrawFrontEnd() is the one caller, and the name is
     * inferred.
     *
     * @ghidraAddress 0x001716d0
     */
    static void RenderBurnTextures();

    /**
     * Resolve one numbered persona burn texture out of the render manager.
     *
     * The name is formatted as `persona_texburn_texture_%d.tex` from one past the index, so index
     * zero resolves `persona_texburn_texture_1.tex`. A name the manager does not recognise produces
     * a null result, as does an object that is not a Rnd::Tex. RenderBurnTextures() expands it
     * inline. The name is inferred.
     *
     * @param nIndex Zero-based texture index.
     * @return The texture, or null when the manager has no such object.
     * @ghidraAddress 0x001712c0
     */
    static Rnd::Tex *FindPersonaBurnTexture(int nIndex);

    /**
     * Copy another appearance through operator=().
     *
     * The image has no caller. The name is inferred.
     *
     * @param other The appearance to copy.
     * @ghidraAddress 0x00174458
     */
    void CopyFrom(const FreqAppearance &other);

    /**
     * Fill a compact record from this appearance.
     *
     * The record is marked valid and takes the skill status, the username cut to 12 characters,
     * and the packed parts of the detail object. Only the low byte of the part count survives. The
     * image has no caller, and the name is inferred.
     *
     * @param pRecord The record to fill.
     * @ghidraAddress 0x00174930
     */
    void Pack(Record *pRecord);

    /**
     * Replace this appearance from a compact record.
     *
     * A record that is not marked valid changes nothing. The image has no caller, and the name is
     * inferred.
     *
     * @param record The record to read.
     * @ghidraAddress 0x001749e8
     */
    void Unpack(const Record &record);

    /**
     * Record the skill status.
     *
     * Defined in the header. The one out-of-line copy is never called, and
     * MetPersonaData::UpdateSkillStatus() expands the store where it is used.
     *
     * @param nStatus The status, 0 through 4.
     * @ghidraAddress 0x00174478
     */
    void SetSkillStatus(int nStatus) {
        mUnknown0c = nStatus;
    }

    /**
     * Report the skill status.
     *
     * Defined in the header. The out-of-line copy's one caller is the uncalled MetPersonaData
     * forwarder at `0x0032e258`.
     *
     * @return The status, 0 through 4.
     * @ghidraAddress 0x001747a0
     */
    int GetSkillStatus() const {
        return mUnknown0c;
    }

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
    // 0x001744f8. Fill bytes 7 through 14 of pDest with 0x80, copy seven source bytes with each
    // zero replaced by 0xff, and set bit n of byte 7 for each zero at position n. The image has no
    // caller, and the name is inferred.
    static void EncodeNonZeroBytes(const unsigned char *pSource, unsigned char *pDest);

    FreqAppearanceDetail *mDetail; // +0x08 owned, 0xb0 bytes
    int mUnknown0c;                // +0x0c the skill status
};
