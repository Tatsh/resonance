#pragma once

#include <cstddef>
#include <iostream>
#include <vector>

#include "app/msgsource.h"

class IBStream;
class OBStream;
class Phrase;
class PlayMap;
class Player;

/**
 * Store of one phrase per step of a play map, and one value per step.
 *
 * `14PhraseDatabase` in the RTTI descriptor, with MsgSource as its one base. The object is 0x30
 * bytes: the MsgSource subobject over `+0x00` through `+0x13`, then the three members below. Its
 * vtable at `0x007e1db0` runs four entries (the type function, the destructor, and the retained
 * MsgSource::AddSink() and MsgSource::RemoveSink()). The class declares no virtual beyond the
 * destructor.
 *
 * The constructor sizes the phrase vector to the last step of the play map and the value vector to
 * one less than the step count. A phrase is addressed either by index or by a song position, which
 * PlayMap::Slot5() maps to an index.
 *
 * Every member is private. Only this class's routines address them.
 */
class PhraseDatabase : public MsgSource {
public:
    /**
     * Allocate a database from the tagged heap under the tag `PhraseDatabase`.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x001b8cc8
     */
    void *operator new(size_t nSize);

    /**
     * Release a database to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x001b8ce8
     */
    void operator delete(void *pBlock);

    /**
     * Construct an empty database over a play map.
     *
     * @param pMap The play map whose steps size both vectors.
     * @ghidraAddress 0x001b72d8
     */
    PhraseDatabase(PlayMap *pMap);

    /**
     * Release every phrase.
     *
     * @ghidraAddress 0x001b75e8
     */
    virtual ~PhraseDatabase();

    /**
     * Give every step a phrase owned by one player, creating the phrases that do not exist yet.
     *
     * @param pPlayer The owner.
     * @ghidraAddress 0x001b77a0
     */
    void SetOwners(Player *pPlayer);

    /**
     * Write the database behind a version byte of 1.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001b7898
     */
    void Save(OBStream &stream);

    /**
     * Read the database back after releasing its contents.
     *
     * The version byte is read and ignored. Neither vector is resized, so the stream is trusted to
     * match the play map.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x001b7a28
     */
    void Load(IBStream &stream);

    /**
     * Release every phrase and zero every value.
     *
     * @ghidraAddress 0x001b8d60
     */
    void Clear();

    /**
     * Return every existing phrase to the stand-in player, logging entry and exit on the console.
     *
     * The routine writes its own name in both log lines.
     *
     * @ghidraAddress 0x001b8dc8
     */
    void ClearOwners();

    /**
     * @param nBar The bar, mapped through PlayMap::Slot5().
     * @return The phrase of the mapped bar, or null.
     * @ghidraAddress 0x001b8e50
     */
    Phrase *GetPhraseAt(int nBar);

    /**
     * @param nIndex The step.
     * @return The phrase of the step, or null.
     * @ghidraAddress 0x001b8e98
     */
    Phrase *GetPhrase(int nIndex);

    /**
     * Store a phrase at the step the play map places a song position in.
     *
     * @param pPhrase The phrase, or null.
     * @param nTick The song position, in MIDI ticks.
     * @ghidraAddress 0x001b8eb0
     */
    void SetPhraseAt(Phrase *pPhrase, int nTick);

    /**
     * Store a phrase at a step, releasing the phrase it replaces and taking a reference.
     *
     * @param pPhrase The phrase, or null.
     * @param nIndex The step.
     * @ghidraAddress 0x001b8f08
     */
    void SetPhrase(Phrase *pPhrase, int nIndex);

    /**
     * Release the phrase at the step the play map places a song position in.
     *
     * @param nTick The song position, in MIDI ticks.
     * @ghidraAddress 0x001b8f78
     */
    void ClearPhraseAt(int nTick);

    /**
     * Release the phrase at a step.
     *
     * @param nIndex The step.
     * @ghidraAddress 0x001b8fc0
     */
    void ClearPhrase(int nIndex);

    /**
     * Give the phrase at the step the play map places a song position in an owner.
     *
     * @param pPlayer The owner.
     * @param nTick The song position, in MIDI ticks.
     * @ghidraAddress 0x001b9018
     */
    void SetOwnerAt(Player *pPlayer, int nTick);

    /**
     * Give the phrase at a step an owner, creating the phrase when the step has none.
     *
     * @param pPlayer The owner.
     * @param nIndex The step.
     * @ghidraAddress 0x001b9070
     */
    void SetOwner(Player *pPlayer, int nIndex);

    /**
     * @param nIndex The step.
     * @return The owner of the phrase at the step, or the stand-in player when the step has none.
     * @ghidraAddress 0x001b9130
     */
    Player *GetOwner(int nIndex);

    /**
     * Set the byte at `+0x28` of the phrase at a step, when the step has one.
     *
     * @param nIndex The step.
     * @param cValue The byte.
     * @ghidraAddress 0x001b9158
     */
    void SetPhraseByte(int nIndex, char cValue);

    /**
     * @param nIndex The step.
     * @return The byte at `+0x28` of the phrase at the step, or zero when the step has none.
     * @ghidraAddress 0x001b9178
     */
    unsigned char GetPhraseByte(int nIndex);

    /**
     * @param nBar The bar.
     * @return The value of the step at or before the bar PlayMap::Slot5() maps nBar to.
     * @ghidraAddress 0x001b91a0
     */
    long long *GetStepValue(int nBar);

    /**
     * Write every phrase to a diagnostic stream, eight to a line, with `[null] ` for an empty
     * step.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001b91f0
     */
    void Print(std::ostream &stream);

private:
    std::vector<Phrase *> mPhrases;    // +0x14, one per step, reference counted
    std::vector<long long> mUnknown20; // +0x20, one per step boundary, four bytes on the wire
    PlayMap *mMap;                     // +0x2c
};
