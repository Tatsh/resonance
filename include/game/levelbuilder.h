#pragma once

#include <vector>

#include "app/attachment.h"
#include "game/leveldata.h"

/**
 * Accumulator a MIDI conversion fills, and the one implementation of LevelData.
 *
 * `12LevelBuilder` in the RTTI descriptor at `0x008efc40`, with LevelData as its one base at
 * offset 0. Its table is at `0x007e79f8` and has ten entries with a zero terminator at index 10,
 * the same length as the base table, so the class adds no virtual. It implements all eight of
 * LevelData's pure slots and supplies the destructor.
 *
 * LevelConverter drives it. LevelConverter::Convert() stores the builder at its own `+0x50`, each
 * MIDI event handler forwards to AddEvent() with the status byte for the event, and
 * Mid::Receiver::AllDone() forwards to Finish().
 *
 * The object is at least 0x38 bytes. Three vectors of pointers occupy `+0x04` through `+0x27`, and
 * the destructor clears each one with a std::for_each over the deleting function at `0x001ec328`,
 * so all three manage their elements. That function deletes a TrackData, which is what fixes the
 * element type of all three. The two scalars at `+0x30` and `+0x34` are managed as well,
 * the first through Attachment::Release() and the second through its own table slot 1.
 *
 * The forwarding members below all reach the object at `+0x2c`, which the destructor does not
 * release. That object is the one the conversion appends to, and its routines sit in the same
 * address range as TrackData's own destructor and appenders, so it is a TrackData.
 *
 * Every method name here is inferred from the body behind it.
 */
class LevelBuilder : public LevelData {
public:
    /**
     * @ghidraAddress 0x001eafe8
     */
    virtual ~LevelBuilder();

    /**
     * @ghidraAddress 0x001ec430
     */
    virtual int TrackCount();

    /**
     * @ghidraAddress 0x001ec448
     */
    virtual int UnknownCount();

    /**
     * @ghidraAddress 0x001ec6d0
     */
    virtual TrackData *OwnTrack();

    /**
     * @ghidraAddress 0x001ec6f0
     */
    virtual TrackData *TrackAt(int nIndex);

    /**
     * @ghidraAddress 0x001ec708
     */
    virtual TrackData *UnknownAt(int nIndex);

    /**
     * @ghidraAddress 0x001ec478
     */
    virtual Attachment *OnUnknownSlot7();

    /**
     * @ghidraAddress 0x001ec480
     */
    virtual PlayMap *OnUnknownSlot8();

    /**
     * @ghidraAddress 0x001ec738
     */
    virtual void OnUnknownSlot9();

    /**
     * Append one MIDI event to the track being filled.
     *
     * The status and the channel arrive separately and the body combines them with a bitwise or
     * before forwarding, so the appender receives the whole status byte. The body is not written
     * yet, because the appender at `0x001d4088` has no recovered name.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nStatus The MIDI status byte without its channel, 0x80 through 0xe0.
     * @param nData1 The first data byte.
     * @param nData2 The second data byte, or zero for a one-byte event.
     * @param nChannel The channel, which the body ors into the status.
     * @ghidraAddress 0x001ec4c8
     */
    void AddEvent(int nTick, int nStatus, int nData1, int nData2, int nChannel);

    /**
     * Unrecovered. A two-argument forwarder to the routine at `0x001d3d10` on the track at
     * `+0x2c`.
     *
     * LevelConverter::EndTrack() calls it with the name map it built for the track. The body is not
     * written yet.
     *
     * @param nFirst The first argument, forwarded unchanged.
     * @param pSecond The second argument, forwarded unchanged.
     * @ghidraAddress 0x001ec560
     */
    void OnUnknownForwarder001ec560(int nFirst, void *pSecond);

    /**
     * Unrecovered. A two-argument forwarder to the routine at `0x001d7758` on the track at
     * `+0x2c`.
     *
     * LevelConverter::EndTrack() calls it with two zeroes. The body is not written yet.
     *
     * @param nFirst The first argument, forwarded unchanged.
     * @param nSecond The second argument, forwarded unchanged.
     * @ghidraAddress 0x001ec580
     */
    void OnUnknownForwarder001ec580(int nFirst, int nSecond);

    /**
     * Replace the object at `+0x30` once the whole file has been delivered.
     *
     * The body releases the object already there, allocates 0x28 bytes, constructs the replacement
     * through the routine at `0x0052d118` with the second parameter, and stores it.
     *
     * The one caller, LevelConverter::AllDone() at `0x001ea570`, sets only the object register, so
     * the second parameter arrives with whatever the caller happened to leave in that register.
     * That is faithful rather than a reconstruction slip. The body is not written yet.
     *
     * @param nUnused The first parameter. No instruction in the body reads it.
     * @param pSource The value the replacement is constructed from.
     * @ghidraAddress 0x001ec5f8
     */
    void Finish(int nUnused, void *pSource);

    /**
     * Apply the routine at `0x001d4308` to every track in the collection.
     *
     * LevelConverter::Convert() calls it once the reader has been built and before the file is
     * read. The body is not written yet, because that routine has no recovered name. It is a
     * TrackData member, from its address range and from its one call site.
     *
     * @ghidraAddress 0x001ec680
     */
    void PrepareTracks();

private:
    // All three vectors manage their elements. The destructor clears each with a std::for_each
    // over the deleting function at 0x001ec328.
    std::vector<TrackData *> mTracks; // +0x04
    // What distinguishes this collection from mTracks is unrecovered.
    std::vector<TrackData *> mUnknown10; // +0x10
    // No slot of LevelData reads this collection.
    std::vector<TrackData *> mUnknown1c; // +0x1c
    // Deleted by the destructor through TrackData's own destructor at 0x001d3900.
    TrackData *mOwnTrack; // +0x28
    // The track the forwarding members append to. The destructor does not release it.
    TrackData *mCurrentTrack; // +0x2c
    // Released by the destructor through Attachment::Release(), which is what types it. Finish()
    // replaces it. Which subclass it is remains unrecovered.
    Attachment *mUnknown30; // +0x30
    // Deleted by the destructor through its own table slot 1. Its vptr sits at its own +0x38.
    PlayMap *mUnknown34; // +0x34
};
