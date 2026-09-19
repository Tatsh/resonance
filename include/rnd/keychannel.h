#pragma once

#include "os/failsink.h"

namespace Rnd {

/**
 * Write a channel of colour keyframes to the engine text sink.
 *
 * Which element type each of these two takes is established by use rather than by the element,
 * whose layout is unrecovered. Rnd::MatAnim dumps its emissive, ambient, diffuse, and specular
 * channels through this one, Rnd::LightAnim its three colour channels, and Rnd::ParticleSysAnim
 * its two spawn colour channels.
 *
 * The return is the sink, by a weaker argument than usual. Neither routine writes the return
 * register deliberately at its exit; the value falls through from an inner sink call whose own
 * contract is to return the sink, which is what a tail call on the sink compiles to. Three
 * independent callers then consume the result as the sink. The alternative reading is that these
 * return nothing and every caller's reuse is an accident, which three callers make unlikely
 * without making it impossible.
 *
 * @param sink The text sink.
 * @param nChannel The channel, a `std::list` sentinel held in one word.
 * @return The sink.
 * @ghidraAddress 0x004d8de8
 */
FailSink &DumpColorKeys(FailSink &sink, int nChannel);

/**
 * Write a channel of scalar keyframes to the engine text sink.
 *
 * The companion of DumpColorKeys(), reached for a channel whose keys are single floats.
 * Rnd::MatAnim dumps its alpha channel through it and Rnd::ParticleSysAnim its emission rate.
 *
 * @param sink The text sink.
 * @param nChannel The channel, a `std::list` sentinel held in one word.
 * @return The sink.
 * @ghidraAddress 0x004d8f08
 */
FailSink &DumpFloatKeys(FailSink &sink, int nChannel);

} // namespace Rnd
