#pragma once

#include <cstddef>
#include <iostream>
#include <vector>

#include "app/attachment.h"
#include "mid/mbt.h"
#include "mid/tickobj.h"

class IBStream;
class MultiMuse;
class MuseMsg;
class OBStream;
class Player;

/**
 * One phrase of a track, with its gems, its player, and the messages and values scheduled across
 * it.
 *
 * `6Phrase` in the RTTI descriptor, with Attachment as its one base. The allocation in
 * `operator>>(IBStream &, Phrase *&)` measures the object at 0x2c bytes. Its vtable at
 * `0x007e1b68` runs three entries (the type function, the destructor, and the retained
 * Attachment::Destroy()). The destructor is the only virtual the class declares.
 *
 * The constructor and the destructor together account for every byte of the member map. Print()
 * labels three of the members `gems: `, `pl: `, and `X: `, and Save() and Load() transfer the
 * same three in the same order behind a version byte of 2.
 *
 * Every member is private. AxePhraseMaker::StartPhrase() writes mPlayer at `0x0019bd7c`.
 * PhraseDatabase writes mPlayer and reads and writes mUnknown28 directly,
 * PhraseMgr::GetPhraseOwner() reads mPlayer, TrackData::AddPhrases() walks mGems directly,
 * PhrasePlayer reads mPlayer, mGems, and mMuse when it plays a bar, the image exposes no accessor,
 * and friend declarations model that access. Promoting the members to public fits the image equally
 * well.
 */
class Phrase : public Attachment {
    friend class AxePhraseMaker;
    friend class PhraseDatabase;
    friend class PhraseMgr;
    friend class PhrasePlayer;
    friend class TrackData;

public:
    /**
     * One gem of the phrase at one song position.
     *
     * The record is twelve bytes. `PhraseMgr::PostGemMsg()` passes a GemPacket's `loc`, `gem`, and
     * `trans` values to AddGem() in member order, and the member names follow the labels
     * GemPacket::Fields::Print() writes for them. The type name is inferred. The record has no
     * descriptor, allocation tag, or literal.
     *
     * On the wire the position travels as two bytes, mGem as one unsigned byte, and mTrans as one
     * signed byte.
     *
     * The fill value Load() hands to `resize()` sets the position alone, which is Mid::MBT's
     * default constructor at work. The inline comparison is recovered from its expansion in the
     * gem search AddGem() performs.
     */
    struct Gem {
        /**
         * Order two gems by position.
         *
         * @param other The gem to compare against.
         * @return Whether this gem comes first.
         */
        bool operator<(const Gem &other) const {
            return mPosition.mTick < other.mPosition.mTick;
        }

        /**
         * Compare two gems member by member, in declaration order.
         *
         * Recovered from its expansion in PhraseMgr::PhrasesMatch().
         *
         * @param other The gem to compare against.
         * @return Whether all three members are equal.
         */
        bool operator==(const Gem &other) const {
            return mPosition.mTick == other.mPosition.mTick && mGem == other.mGem &&
                   mTrans == other.mTrans;
        }

        Mid::MBT mPosition; /*!< Song position of the gem. +0x00 */
        int mGem;           /*!< Which gem. +0x04 */
        int mTrans;         /*!< Transposition of the gem. +0x08 */
    };

    /**
     * Allocate a phrase from the tagged heap under the tag `Phrase`.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x001b6a28
     */
    void *operator new(size_t nSize);

    /**
     * Release a phrase to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x001b6a48
     */
    void operator delete(void *pBlock);

    /**
     * Construct an empty phrase owned by the stand-in player.
     *
     * @ghidraAddress 0x001b4940
     */
    Phrase();

    /**
     * Release the message sequence, when one exists, and both vectors.
     *
     * @ghidraAddress 0x001b4998
     */
    virtual ~Phrase();

    /**
     * Place a gem at a song position, replacing any gem already there.
     *
     * A gem after the last one is appended. Otherwise the gem is inserted at its sorted position,
     * and an existing gem at the same position is removed first.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param nGem The gem.
     * @param nTrans The transposition.
     * @return The replaced gem's mGem, or -1 when no gem was replaced.
     * @ghidraAddress 0x001b4b30
     */
    int AddGem(int nTick, int nGem, int nTrans);

    /**
     * Schedule a copy of a message at a song position, creating the sequence on first use.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param pMsg The message. The sequence stores a clone of it.
     * @ghidraAddress 0x001b4cd8
     */
    void AddMuseMsg(int nTick, MuseMsg *pMsg);

    /**
     * Write the phrase to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001b4d68
     */
    void Print(std::ostream &stream);

    /**
     * Write the phrase behind a version byte of 2.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001b5018
     */
    void Save(OBStream &stream);

    /**
     * Read the phrase back.
     *
     * Version 1 and any version below 2 are reported through Fatal(). The player arrives as its
     * identifier, and an identifier the table does not resolve falls back to the table's first
     * player.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x001b51f8
     */
    void Load(IBStream &stream);

    /**
     * Add a value at a song position, keeping the values sorted by position.
     *
     * @param nTick The song position, in MIDI ticks.
     * @param flValue The value.
     * @ghidraAddress 0x001b6d88
     */
    void AddValue(int nTick, float flValue);

    /**
     * Report the value in force at a song position.
     *
     * @param nTick The song position, in MIDI ticks.
     * @return The value of the last entry at or before nTick, or 0.5 when none exists.
     * @ghidraAddress 0x001b6db0
     */
    float GetValue(int nTick);

private:
    // Written behind the value list, as the count and the entries in a narrower form than the
    // gems use.
    // 0x001b5500
    void SaveValues(OBStream &stream);

    // 0x001b55f8
    void LoadValues(IBStream &stream);

    std::vector<Gem> mGems;               // +0x08, labelled `gems: `
    Player *mPlayer;                      // +0x14, labelled `pl: `
    MultiMuse *mMuse;                     // +0x18, created on first use
    std::vector<TickObj<float> > mValues; // +0x1c, labelled `X: `
    char mUnknown28;                      // +0x28
};

/**
 * Write one gem as a two-byte position and two single bytes.
 *
 * Save() expands the same transfer inline, and no caller of this copy exists.
 *
 * @param stream The stream to write to.
 * @param gem The gem.
 * @return The stream.
 * @ghidraAddress 0x001b6ba0
 */
OBStream &operator<<(OBStream &stream, const Phrase::Gem &gem);

/**
 * Read one gem back.
 *
 * @param stream The stream to read from.
 * @param gem The gem to fill.
 * @return The stream.
 * @ghidraAddress 0x001b6c48
 */
IBStream &operator>>(IBStream &stream, Phrase::Gem &gem);

/**
 * Write one gem to a diagnostic stream as `[position gem trans]`.
 *
 * @param stream The stream to write to.
 * @param gem The gem.
 * @return The stream.
 * @ghidraAddress 0x001b6cf8
 */
std::ostream &operator<<(std::ostream &stream, Phrase::Gem &gem);

/**
 * Write a phrase behind the presence byte `1`.
 *
 * @param stream The stream to write to.
 * @param phrase The phrase.
 * @return The stream.
 * @ghidraAddress 0x001b6e28
 */
OBStream &operator<<(OBStream &stream, Phrase &phrase);

/**
 * Read a phrase written by `operator<<(OBStream &, Phrase &)`, discarding its presence byte.
 *
 * @param stream The stream to read from.
 * @param phrase The phrase to fill.
 * @return The stream.
 * @ghidraAddress 0x001b6e88
 */
IBStream &operator>>(IBStream &stream, Phrase &phrase);

/**
 * Write an optional phrase as the byte `0` for none or the byte `1` followed by the phrase.
 *
 * PhrasePacket::Save() and PhraseDatabase use this form.
 *
 * @param stream The stream to write to.
 * @param pPhrase The phrase, or null.
 * @return The stream.
 * @ghidraAddress 0x001b6ee0
 */
OBStream &operator<<(OBStream &stream, Phrase *pPhrase);

/**
 * Read an optional phrase, allocating a new one unless the presence byte is `0`.
 *
 * @param stream The stream to read from.
 * @param pPhrase Receives the new phrase, or null.
 * @return The stream.
 * @ghidraAddress 0x001b6f70
 */
IBStream &operator>>(IBStream &stream, Phrase *&pPhrase);

/**
 * Write a phrase to a diagnostic stream through Phrase::Print().
 *
 * @param stream The stream to write to.
 * @param phrase The phrase.
 * @return The stream.
 * @ghidraAddress 0x001b7038
 */
std::ostream &operator<<(std::ostream &stream, Phrase &phrase);
