#pragma once

#include <cstddef>
#include <iostream>
#include <vector>

#include "game/controllerconfig.h"
#include "met/gameoptions.h"
#include "met/metsonglists.h"
#include "os/hxstr.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Settings shared by every player and saved to the memory card apart from any persona.
 *
 * `14GlobalSettings` in the RTTI descriptor, a leaf class with no base. Following the g++ 2.x
 * layout for a class with no base, the vptr sits after the data at `+0x7c`, and the object is 0x80
 * bytes, which the allocation in Create() fixes. The vtable at `0x007db970` runs the type function
 * at `0x0018b948`, the destructor, Save(), and Load().
 *
 * The one instance lives at `0x0067ea38`. MetRenderer's constructor creates it through Create(),
 * and about a hundred routines reach it through shared().
 *
 * The translation unit spans `0x00187c00` to `0x0018bc28`. Besides the members below, it holds
 * template library emissions (`0x00188e88`, `0x00189060`, `0x0018a158`, `0x0018a840`,
 * `0x0018adb8`, `0x0018b0e8`, `0x0018b528`, and `0x0018bc28`), copies of routines defined
 * elsewhere, and the printer at `0x00187c60`, which writes a network server record rather than
 * this class. None of those is declared here.
 */
class GlobalSettings {
public:
    /** The number of controller mappings. */
    static constexpr int kControllerCount = 4;

    /**
     * Allocate an instance from the tagged heap under the tag `GlobalSettings`.
     *
     * @param nSize The object size, which the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x0018b988
     */
    void *operator new(size_t nSize);

    /**
     * Release an instance to the tagged heap.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x0018b9a8
     */
    void operator delete(void *pBlock);

    /**
     * Start with the default mappings, options, and names, the address `127.0.0.2`, the port
     * `2000`, and one memory-card location, `1`.
     *
     * @ghidraAddress 0x00187d00
     */
    GlobalSettings();

    /**
     * @ghidraAddress 0x00188370
     */
    virtual ~GlobalSettings();

    /**
     * Write the settings.
     *
     * Slot 2. The record version 4 comes first, then the address, the port, the four mappings,
     * the options, the name count and the names, and the tutorial flag.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001885d0
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the settings back.
     *
     * Slot 3. A record older than version 2 has another string where the port belongs, which is
     * discarded in favour of the default port. A version 0 record has one mapping. A record older
     * than version 3 has an older name list, which SkipLegacyNames() discards. Only a version 4
     * record has the tutorial flag.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x001887d0
     */
    virtual void Load(IBStream &stream);

    /**
     * Report the instance.
     *
     * @return The instance, or null outside the lifetime of the front-end renderer.
     * @ghidraAddress 0x0018b9c8
     */
    static GlobalSettings *shared();

    /**
     * Allocate the instance.
     *
     * @ghidraAddress 0x0018b9d8
     */
    static void Create();

    /**
     * Delete the instance and clear the pointer shared() reports.
     *
     * @ghidraAddress 0x0018ba48
     */
    static void Destroy();

    /**
     * Replace the names with a list.
     *
     * The list arrives by value. Only as many names as the list has are replaced. The image has
     * no caller.
     *
     * @param names The names.
     * @ghidraAddress 0x00188cc0
     */
    void SetNames(std::vector<HxStr> names);

    /**
     * Write the address, the port, the four mapping labels, and the tutorial flag to a stream.
     *
     * The image has no caller.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0018ba90
     */
    void Print(std::ostream &stream);

    /**
     * Copy another instance's settings, except the memory-card locations.
     *
     * Exactly twelve names are copied, whatever either list holds, and the routine returns
     * nothing. The image has no caller.
     *
     * @param other The settings to copy.
     * @ghidraAddress 0x0018bb50
     */
    void operator=(const GlobalSettings &other);

    /** The four controller mappings. InputMap::Rebuild() reads them. +0x00 */
    ControllerConfig mControllers[kControllerCount];
    /** The game options. +0x30 */
    GameOptions mGameOptions;

private:
    // 0x00188b90. Read and discard the name list of a record older than version 3. The body does
    // not read this object.
    void SkipLegacyNames(IBStream &stream);

    // The default names, which the constructor copies into mNames. +0x3c
    std::vector<HxStr> *mDefaultNames;
    // The names Save() writes. +0x40
    std::vector<HxStr> mNames;
    // The network address, labelled `Net IP Address` by Print(). +0x4c
    HxStr mNetAddress;
    // The network port as text, labelled `Net Port` by Print(). +0x54
    HxStr mNetPort;

public:
    /** Non-zero once the tutorial is complete, labelled by Print(). +0x5c */
    int mTutorialComplete;
    /**
     * The memory-card locations. MetStageFinishScreen's slot 5 and FirstCardSlotName() read
     * them. +0x60
     */
    std::vector<CardSlot> mCardSlots;
    /** Starts at 256. MinimumSaveSpaceMCT reads it. +0x6c */
    int mUnknown6c;

private:
    int mUnknown70; // +0x70, starts at 60
    int mUnknown74; // +0x74, starts at 24
    int mUnknown78; // +0x78, starts at 0
};
