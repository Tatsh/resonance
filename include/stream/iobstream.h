#pragma once

#include "stream/ibstream.h"
#include "stream/obstream.h"

/**
 * Bidirectional byte stream interface.
 *
 * `9IOBStream` in the RTTI descriptor at `0x008ef570`. The base list at `0x008242c8` records two
 * bases, IBStream at offset 0 and OBStream at offset 4, both public and non-virtual. The
 * descriptor is built only inside IOBMemStream's type function at `0x004ed780`, and no vtable in
 * the image addresses a type function for it, so the class is abstract and is never instantiated
 * on its own.
 *
 * Recovery stops at the two bases. IOBMemStream is the only subclass, and its primary vtable adds
 * three slots beyond the eight IBStream declares. Whether those three were declared here or on
 * IOBMemStream cannot be settled from the image, because an abstract class with no vtable
 * contributes no evidence of its own slot count. They are declared on IOBMemStream, which is the
 * reading that assumes the least. IOBPreallocMemStream does not help, since it derives from
 * IBStream and OBStream directly and adds two slots of its own with different meanings.
 *
 * Fail() is declared with the same signature on both bases, and the one override in a derived
 * class serves both slots.
 */
class IOBStream : public IBStream, public OBStream {};
