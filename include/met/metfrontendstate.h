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
     * @ghidraAddress 0x00215488
     */
    MetFrontEndState();

    /**
     * Delete every persona and release the screen name.
     *
     * @ghidraAddress 0x00215568
     */
    ~MetFrontEndState();

    /**
     * Report the instance Create() allocated.
     *
     * @return The instance, or null outside the lifetime of the front-end renderer.
     * @ghidraAddress 0x00217f30
     */
    static MetFrontEndState *shared();

    /**
     * Allocate the instance.
     *
     * MetRenderer's constructor is the one caller. An existing instance is overwritten rather than
     * released.
     *
     * @ghidraAddress 0x00217f40
     */
    static void Create();

    /**
     * Delete the instance and clear the pointer shared() reports.
     *
     * MetRenderer's destructor is the one caller.
     *
     * @ghidraAddress 0x00217fa0
     */
    static void Destroy();

    /**
     * Clear every flag.
     *
     * The persona vector and the screen name are not touched.
     *
     * @ghidraAddress 0x00217fd8
     */
    void Reset();

    /**
     * Report the first persona the game manager records.
     *
     * The body copies the game manager's whole persona vector and reads the first element of the
     * copy. It does not read this object.
     *
     * @return The persona, or null when the game manager records none.
     * @ghidraAddress 0x002156b0
     */
    MetPersonaData *GetFirstPersona();

    /** Personas this object owns. The destructor deletes each one. +0x00 */
    std::vector<MetPersonaData *> mUnknown00;
    /** Tested by about a dozen screens and cleared by MetMemDetectScreen. +0x0c */
    int mUnknown0c;
    /** Set by the configuration screens and cleared by MetMainScreen::EnterAndShow(). +0x10 */
    int mUnknown10;
    /** Set to 1 by MetLogoScreen slot 3. MetArenasScreen slot 5 reads it. +0x14 */
    int mUnknown14;
    /** Cleared by MetTutorialScreen slot 5 after it copies the value into mUnknown1c. +0x18 */
    int mUnknown18;
    /** The previous value of mUnknown18. +0x1c */
    int mUnknown1c;
    int mUnknown20; /*!< Cleared by Reset(). No reader is identified. +0x20 */
    /** The name of the screen to return to, which several exit hooks record. +0x24 */
    HxStr mUnknown24;
    /** Written by MetLocNumPlayScreen slot 36 and read back by its slot 5. +0x2c */
    int mUnknown2c;
};
