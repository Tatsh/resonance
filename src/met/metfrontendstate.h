#pragma once

#include <vector>

#include "os/hxstr.h"

class MetPersonaData;

/**
 * Front-end state shared by every screen for the lifetime of the front-end renderer.
 *
 * The class is not polymorphic and emits no RTTI descriptor, and no literal or file path in the
 * image records it, so the name is inferred from its use. MetRenderer's constructor creates the
 * one instance through Create() and its destructor releases it through Destroy(). About a hundred
 * routines across the front-end screens reach it through shared() and read or write its flags
 * directly, which is why every member is public.
 *
 * The object is 0x30 bytes, which the allocation in Create() fixes. The constructor starts the
 * persona vector and the screen name empty and then runs Reset() on every other member.
 */
class MetFrontEndState {
public:
    /**
     * Start with no persona, an empty screen name, and every flag clear.
     *
     * @ghidraAddress NTSC-U/C: 0x00215488
     * @ghidraAddress PAL: 0x0021f0b8
     */
    MetFrontEndState();

    /**
     * Delete every persona and release the screen name.
     *
     * @ghidraAddress NTSC-U/C: 0x00215568
     * @ghidraAddress PAL: 0x0021f1b0
     */
    ~MetFrontEndState();

    /**
     * Report the instance Create() allocated.
     *
     * @return The instance, or null outside the lifetime of the front-end renderer.
     * @ghidraAddress NTSC-U/C: 0x00217f30
     * @ghidraAddress PAL: 0x00221bb8
     */
    static MetFrontEndState *shared();

    /**
     * Allocate the instance.
     *
     * MetRenderer's constructor is the one caller. An existing instance is overwritten rather than
     * released.
     *
     * @ghidraAddress NTSC-U/C: 0x00217f40
     * @ghidraAddress PAL: 0x00221bc8
     */
    static void Create();

    /**
     * Delete the instance and clear the pointer shared() reports.
     *
     * MetRenderer's destructor is the one caller.
     *
     * @ghidraAddress NTSC-U/C: 0x00217fa0
     * @ghidraAddress PAL: 0x00221c28
     */
    static void Destroy();

    /**
     * Clear every flag.
     *
     * The persona vector and the screen name are not touched.
     *
     * @ghidraAddress NTSC-U/C: 0x00217fd8
     * @ghidraAddress PAL: 0x00221c60
     */
    void Reset();

    /**
     * Report the first persona the game manager records.
     *
     * The body copies the game manager's whole persona vector and reads the first element of the
     * copy. It does not read this object.
     *
     * @return The persona, or null when the game manager records none.
     * @ghidraAddress NTSC-U/C: 0x002156b0
     * @ghidraAddress PAL: 0x0021f310
     */
    MetPersonaData *GetFirstPersona();

    /** Personas this object owns. The destructor deletes each one. +0x00 */
    std::vector<MetPersonaData *> mPersonas;
    /**
     * Set while the game saves to a memory card. MetMemDetectScreen sets it once a card is found
     * and clears it when the user continues without one, and about a dozen screens test it before
     * offering a save. +0x0c
     */
    int mUsingMemcard;
    /**
     * Set by the configuration screens when a setting changed while mUsingMemcard is set, and
     * cleared by MetMainScreen::EnterAndShow() once it has saved. +0x10
     */
    int mSettingsDirty;
    /**
     * Set to 1 by MetLogoScreen::RecordUnlock(). MetArenasScreen slot 5 and MetSoloStagesScreen
     * read it to unlock every arena and stage. +0x14
     */
    int mUnlockAll;
    /**
     * How the front end was departed for a game (the tutorial, a game, or a quit from the pause
     * screen). The screen entered on return acts on it and clears it after copying the value into
     * mLastTransition. +0x18
     */
    int mPendingTransition;
    /** The previous value of mPendingTransition. +0x1c */
    int mLastTransition;
    int mUnusedFlag; /*!< Cleared by Reset(). No reader is identified. +0x20 */
    /** The name of the screen to return to, which several exit hooks record. +0x24 */
    HxStr mReturnScreen;
    /**
     * The number of players MetLocNumPlayScreen slot 36 recorded and its slot 5 selects again.
     * +0x2c
     */
    int mPlayerCount;
};
