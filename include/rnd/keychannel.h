#pragma once

#include <list>

#include "math/color.h"

class FailSink;
namespace Rnd {
class Stream;
}

namespace Rnd {

/**
 * One keyframe of a colour channel.
 *
 * The record is 0x20 bytes and three independent readings agree on it. The list constructor at
 * `0x004ddc48` passes 0x20 as the element size and allocates a 0x30-byte node from slot `+0x14`
 * of the node pool at `0x00667080`. The copy assignment at `0x004d9aa0` moves the quadwords at
 * node `+0x10` and node `+0x20`, which places a 0x20-byte payload at node `+0x10`. And
 * Rnd::LightAnim::SetFrameSelf() loads a quadword from element `+0x00` as the colour and a float
 * from element `+0x10` as the frame.
 *
 * The 0xc bytes after mFrame are read by no instruction in the image. They are either the
 * alignment a 16-byte-aligned element takes on or an unrecovered field, and nothing distinguishes
 * the two.
 *
 * The record is not polymorphic and emits no RTTI descriptor, so the title is inferred rather than
 * recovered. Its member titles follow the element dump at `0x004d8c88`, which writes "(frame:"
 * ahead of mFrame and " value:" ahead of mValue, and they match the keyframe records Rnd::MeshAnim
 * already recovered.
 */
struct ColorKey {
    Color mValue; /*!< Colour the frame interpolates towards. +0x00 */
    float mFrame; /*!< Frame the colour applies at. +0x10 */
};

/**
 * One keyframe of a scalar channel.
 *
 * The record is 8 bytes, which the list constructor at `0x004ddcd0` pins by passing 8 as the
 * element size and allocating a 0x10-byte node from slot `+0x04` of the same node pool. A 4-byte
 * aligned element places its payload at node `+0x08`, and the channel dump at `0x004d8f08` reads
 * the value at node `+0x08` and the frame at node `+0x0c`, which orders the two members. The title
 * is inferred on the same basis as Rnd::ColorKey.
 */
struct FloatKey {
    float mValue; /*!< Value the frame interpolates towards. +0x00 */
    float mFrame; /*!< Frame the value applies at. +0x04 */
};

/**
 * Write a channel of colour keyframes to the engine text sink.
 *
 * Rnd::MatAnim dumps its emissive, ambient, diffuse, and specular channels through this one,
 * Rnd::LightAnim its three colour channels, and Rnd::ParticleSysAnim its two spawn colour
 * channels. The dump opens with the keyframe count from `std::list::size()` and then writes one
 * indexed line per keyframe.
 *
 * @param sink The text sink.
 * @param keys The channel.
 * @return The sink.
 * @ghidraAddress 0x004d8de8
 */
FailSink &DumpColorKeys(FailSink &sink, const std::list<ColorKey> &keys);

/**
 * Write a channel of scalar keyframes to the engine text sink.
 *
 * The companion of DumpColorKeys(), reached for a channel whose keys are single floats.
 * Rnd::MatAnim dumps its alpha channel through it and Rnd::ParticleSysAnim its emission rate.
 *
 * @param sink The text sink.
 * @param keys The channel.
 * @return The sink.
 * @ghidraAddress 0x004d8f08
 */
FailSink &DumpFloatKeys(FailSink &sink, const std::list<FloatKey> &keys);

/**
 * Read a channel of colour keyframes from a `.rnd` stream.
 *
 * Reads the keyframe count, resizes the channel to it, and then reads one element per node.
 *
 * @param stream The stream to read from.
 * @param keys The channel to fill.
 * @return The stream.
 * @ghidraAddress 0x004dd9b0
 */
Stream &ReadColorKeys(Stream &stream, std::list<ColorKey> &keys);

/**
 * Write a channel of colour keyframes to a `.rnd` stream.
 *
 * Writes the keyframe count and then one element per node, in the layout ReadColorKeys() expects.
 *
 * @param stream The stream to write to.
 * @param keys The channel.
 * @return The stream.
 * @ghidraAddress 0x004d9180
 */
Stream &WriteColorKeys(Stream &stream, const std::list<ColorKey> &keys);

/**
 * Write a channel of scalar keyframes to a `.rnd` stream.
 *
 * @param stream The stream to write to.
 * @param keys The channel.
 * @return The stream.
 * @ghidraAddress 0x004d9238
 */
Stream &WriteFloatKeys(Stream &stream, const std::list<FloatKey> &keys);

/**
 * Report the frame of the last keyframe of a channel, and zero for an empty channel.
 *
 * No standalone body survives. Every user inlines it, and the count loop the image emits ahead of
 * the emptiness test is what makes the test a `std::list::size()` comparison rather than a
 * sentinel comparison.
 *
 * @param keys The channel.
 * @return The frame of the last keyframe.
 */
template <class Key>
inline float ChannelEndFrame(const std::list<Key> &keys) {
    if (keys.size() == 0) {
        return 0.0f;
    }
    return keys.back().mFrame;
}

/**
 * Select the keyframe pair bracketing a frame, and the blend between the pair.
 *
 * A frame at or before the first keyframe yields that keyframe with a blend of zero, and a frame
 * at or after the last keyframe yields that keyframe with a blend of one, so a channel clamps
 * rather than extrapolating. The channel has to be non-empty. No standalone body survives; every
 * user inlines it, three times over in each of the three animation classes.
 *
 * @param keys The channel to search.
 * @param flFrame The frame to bracket.
 * @param pFrom The keyframe at or below flFrame.
 * @param pTo The keyframe at or above flFrame.
 * @param flBlend The position of flFrame between the pair, from zero to one.
 */
template <class Key>
inline void SelectKeyPair(
    const std::list<Key> &keys, float flFrame, const Key *&pFrom, const Key *&pTo, float &flBlend) {
    if (flFrame <= keys.front().mFrame) {
        pFrom = &keys.front();
        pTo = &keys.front();
        flBlend = 0.0f;
        return;
    }
    if (keys.back().mFrame <= flFrame) {
        pFrom = &keys.back();
        pTo = &keys.back();
        flBlend = 1.0f;
        return;
    }

    typename std::list<Key>::const_iterator itFrom = keys.begin();
    typename std::list<Key>::const_iterator itTo = itFrom;
    ++itTo;
    while (itTo != keys.end() && flFrame > itTo->mFrame) {
        itFrom = itTo;
        ++itTo;
    }
    pFrom = &*itFrom;
    pTo = &*itTo;
    flBlend = (flFrame - itFrom->mFrame) / (itTo->mFrame - itFrom->mFrame);
}

} // namespace Rnd
