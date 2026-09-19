#pragma once

#include "rnd/animatable.h"

class FailSink;

namespace Rnd {

/**
 * Animation that swaps the texture of a material stage over time.
 *
 * `Q23Rnd7TexAnim` in the RTTI descriptor, with `Rnd::Animatable` as its one public base at offset
 * 0.
 *
 * Recovery is minimal. Only two routines are identified, both of them text dumps, and no member is
 * recovered. The class shares its file with Rnd::Movie, which is why its routines sit far from the
 * rest of the texture code.
 */
class TexAnim : public Animatable {
public:
    /**
     * Write the list of animated texture tracks to the text sink.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x005d15e0
     */
    void DumpTexTrackList(FailSink &sink);

    /**
     * Write the textures of one track to the text sink.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x005d21e0
     */
    void DumpTrackTextures(FailSink &sink);
};

} // namespace Rnd
