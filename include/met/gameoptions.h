#pragma once

#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * The three game option settings GlobalSettings records.
 *
 * The class is not polymorphic and emits no RTTI descriptor. The name is inferred from
 * MetConfigGameOptionsScreen, which builds one, and from GlobalSettings, which embeds one at
 * `+0x30`. The object is 12 bytes, which the GlobalSettings layout fixes. MetPersonaData's Load()
 * builds and reads one as well. The purpose of each setting is not recovered.
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

    int mUnknown00; /*!< Starts at 1. +0x00 */
    int mUnknown04; /*!< Starts at 0. +0x04 */
    int mUnknown08; /*!< Starts at 1. +0x08 */
};
