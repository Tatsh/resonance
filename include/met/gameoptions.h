#pragma once

#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * The three game option settings GlobalSettings records.
 *
 * The class is not polymorphic and emits no RTTI descriptor. The name is inferred from
 * MetConfigGameOptionsScreen, which builds one, and from GlobalSettings, which embeds one at
 * `+0x30`. The object is 12 bytes, which the GlobalSettings layout fixes. MetPersonaData's Load()
 * builds and reads one as well.
 */
class GameOptions {
public:
    /**
     * Start with the first and third settings on and the second off.
     *
     * @ghidraAddress 0x0032e780
     */
    GameOptions();

    /**
     * @ghidraAddress 0x0032e798
     */
    ~GameOptions();

    /**
     * Write the second, the third, and the first setting, in that order.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0032e7c8
     */
    void Save(OBStream &stream);

    /**
     * Read the settings back in the order Save() wrote them.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x0032e810
     */
    void Load(IBStream &stream);

    /**
     * Stereo rather than mono output. Starts at 1. MetConfigGameOptionsScreen labels it `STEREO` or
     * `MONO` and hands it to the synthesiser's SetStereo(). +0x00
     */
    int mStereo;
    /**
     * Offer the expansion pack disc. Starts at 0. The expansion pack cheat toggles it, and
     * MetConfigOptionsButtonsScreen shows its disc-change button only while it is set. +0x04
     */
    int mExpansionPack;
    /**
     * Enable controller force feedback. Starts at 1. Handed to ForceFeedbackMgr::SetEnabled().
     * +0x08
     */
    int mForceFeedback;
};
