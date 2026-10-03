#pragma once

#include <iostream>

#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Settings a game session is started with.
 *
 * Its RTTI descriptor is at `0x0086f628`. It has no base. The compiler places the vptr after the
 * data at `+0x34`, and the class is 0x38 bytes. Its vtable at `0x007db5a0` has four entries and a
 * zero terminator at index 4. The destructor and the two transfer members are the whole set of
 * virtuals.
 *
 * The layout comes from the constructor, the destructor, and the copy constructor together. The
 * constructor zeroes every word from `+0x00` to `+0x30` in ascending order, the destructor releases
 * the buffers at `+0x18`, `+0x0c`, and `+0x04` in reverse declaration order, and the copy
 * constructor at `0x001fc480` copies all ten members in declaration order. The GameManagerImpl
 * destructor corroborates the three strings independently by releasing their buffers at `+0x6c`,
 * `+0x74`, and `+0x80` of a manager whose settings start at `+0x68`.
 *
 * That copy constructor is the compiler-generated one and is therefore not declared here. Two
 * measurements agree. It copies every member in exact declaration order with no logic of its own,
 * and it sits at `0x001fc480` while every hand-written member of the class sits in the `0x00187xxx`
 * run, which places it in the translation unit that copies a GameParams rather than in the one that
 * defines the class.
 *
 * The purpose of most members is not recovered. Three of the ten are public because
 * GameManagerImpl reads and writes them directly with no accessor in the image, and the two names
 * are public because Renderer::LoadLevel() reads them directly, as is the jukebox flag, which
 * Globals::IsJukeboxMode() reads, and the loading flag, which MetStageFinishScreen reads. A friend
 * declaration on this class would fit the image equally well as the promotion. The other three
 * have no traced reader and stay private.
 *
 * mUnsavedString is the one member that neither Save(), Load(), nor operator=() touches, and the
 * compiler-generated copy constructor is the only routine in the image that copies it.
 *
 * This class is declared because several network packets embed one.
 */
class GameParams {
public:
    /**
     * Start with every member clear.
     *
     * @ghidraAddress NTSC-U/C: 0x00187170
     * @ghidraAddress PAL: 0x0018c960
     */
    GameParams();

    /**
     * Copy every member in declaration order.
     *
     * Defaulted rather than written, because the routine at `0x001fc480` is the compiler-generated
     * copy constructor, as the class documentation records. Declaring it is what the destructor and
     * the assignment operator below make necessary: a user-declared destructor deprecates an
     * implicit copy constructor, and the three packets that embed a GameParams copy one.
     *
     * @ghidraAddress NTSC-U/C: 0x001fc480
     * @ghidraAddress PAL: 0x002038c8
     */
    GameParams(const GameParams &other) = default;

    /**
     * Release the three strings.
     *
     * The declaration is first among the class's virtuals, which is what places it at vtable
     * slot 1. The body is empty, and the three releases are the compiler-generated member
     * destructor calls. The GameManagerImpl destructor inlines the whole sequence.
     *
     * @ghidraAddress NTSC-U/C: 0x00187940
     * @ghidraAddress PAL: 0x0018d138
     */
    virtual ~GameParams();

    /**
     * Write the settings to a stream.
     *
     * Slot 2. GameManagerImpl::Save() runs it after writing its own three words.
     *
     * @param pStream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x001871b8
     * @ghidraAddress PAL: 0x0018c9b0
     */
    virtual void Save(OBStream *pStream) const;

    /**
     * Read the settings back from a stream.
     *
     * Slot 3. GameManagerImpl::Load() runs it after reading its own three words. The fields come
     * back in the order Save() wrote them.
     *
     * @param pStream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x00187390
     * @ghidraAddress PAL: 0x0018cb88
     */
    virtual void Load(IBStream *pStream);

    /**
     * Copy every member except mUnsavedString from another instance.
     *
     * Self-assignment is not tested, and the routine returns this instance.
     *
     * @param other The settings to copy.
     * @return This instance.
     * @ghidraAddress NTSC-U/C: 0x00187be8
     * @ghidraAddress PAL: 0x0018d470
     */
    GameParams &operator=(const GameParams &other);

    /**
     * Compare every member except mUnsavedString with another instance, the set operator=() copies.
     *
     * The level name and the arena are compared as strings and the other seven words as integers,
     * in declaration order, stopping at the first difference. The shipped program does not call it,
     * and the title is inferred from the shape.
     *
     * @param other The settings to compare with.
     * @return Whether every compared member is equal.
     * @ghidraAddress NTSC-U/C: 0x00187b20
     * @ghidraAddress PAL: 0x0018d3a8
     */
    bool operator==(const GameParams &other) const;

    /**
     * Write the settings to a diagnostic stream on one line, ending it with a newline.
     *
     * Not virtual. The labels are `GameParams:`, ` level=`, ` arena=`, ` friends=`, then ` game` or
     * ` jam` for a play mode of 1 or anything else, ` difficulty=`, ` constrain-jam` when
     * mConstrainJam is set, and then `netgame=`, `loadinggame=`, and `jukeboxmode=`, each of the
     * last three written with no separating space. SPJoinAcceptPacket, BSLoadLevelPacket, and
     * SCLoadLevelPacket call it from their own Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x00187570
     * @ghidraAddress PAL: 0x0018cd68
     */
    void Print(std::ostream &stream) const;

    /**
     * The level the session plays. +0x00
     *
     * Public because Renderer::LoadLevel() copies it directly to build the level's load path, and
     * the image has no accessor for it.
     */
    HxStr mLevelName;

    /**
     * The arena the session plays in. +0x08
     *
     * Public on the same evidence as mLevelName.
     */
    HxStr mArenaName;

private:
    // Labelled `friends=` by Print(). +0x10
    int mFriends;
    // Neither saved, loaded, assigned, nor compared. Only the copy constructor and the destructor
    // touch it. +0x14
    HxStr mUnsavedString;

public:
    int mPlayMode;   /*!< Driven by GameManagerImpl slots 29 and 31, the play mode. +0x1c */
    int mDifficulty; /*!< Driven by GameManagerImpl slots 28 and 30, the difficulty. +0x20 */

private:
    // Labelled ` constrain-jam` by Print() when set. Load() normalises the transferred word to 0
    // or 1, and every access to this member and the three below, the constructor's clears
    // included, is a full word (sw/lw). +0x24
    int mConstrainJam;

public:
    /**
     * Written by GameManagerImpl::SetGameMode() as the test for `net`, 0 or 1, and labelled
     * `netgame=` by Print(). +0x28
     */
    int mNetGame;

    /**
     * Set while a saved game is loading, labelled `loadinggame=` by Print(). +0x2c
     *
     * Public because MetStageFinishScreen::EnterAndShow() reads it directly, and the image has no
     * GameParams accessor for it.
     */
    int mLoadingGame;

    /**
     * Set for a jukebox session. +0x30
     *
     * Overlay's constructor shows `Jukebox Mode` while it is set, and HudFreq hides its icon.
     * Public because Globals::IsJukeboxMode() reads it directly, and the image has no GameParams
     * accessor for it.
     */
    int mJukeboxMode;
};

/**
 * Report whether a won game plays the win sequence.
 *
 * A free function of this translation unit over a file-scope word at `0x0067e798`. Only this pair
 * reads and writes the word. Overlay::OnWin() is the one caller and plays the win sequence only
 * when the result is non-zero. The name follows the script function `do_win_sequence_cheat`, whose
 * handler sets the word. The title is inferred.
 *
 * @return Non-zero when the win sequence is enabled.
 * @ghidraAddress NTSC-U/C: 0x00187b00
 * @ghidraAddress PAL: 0x0018d388
 */
int GetDoWinSequence();

/**
 * Enable or disable the win sequence.
 *
 * ScriptDoWinSequenceCheat() and PyInvokeDoWinSequenceCheat() pass 1, the MetRenderer constructor
 * and the routine at `0x0036bcb8` pass 0, and MetLoadGameScreen::EnterAndShow() passes 1 or 0 by
 * the result of the call before it. The title is inferred.
 *
 * @param nDoWinSequence Non-zero to enable the win sequence.
 * @ghidraAddress NTSC-U/C: 0x00187b10
 * @ghidraAddress PAL: 0x0018d398
 */
void SetDoWinSequence(int nDoWinSequence);
