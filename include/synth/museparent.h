#pragma once

/**
 * Base of the MUSE note hierarchy that a synthesiser derives from.
 *
 * `10MuseParent` in the RTTI descriptor at `0x0086ebb0`, built through the built-in descriptor
 * constructor with no base list. MuseSynth derives from MsgSink at offset 0 and from this class at
 * offset 4.
 *
 * The class is not reconstructed and no member of it is recovered. It is declared so that
 * MuseSynth records the base the RTTI attests rather than omitting it.
 */
class MuseParent {};
