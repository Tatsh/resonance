#pragma once

#include "script/cxx/standarderror.h"

namespace Py {

/**
 * Exception for a missing attribute, mirroring Python's AttributeError.
 *
 * `Q22Py14AttributeError` in the RTTI descriptor at `0x008eefb8`, with Py::StandardError at offset
 * 0 as its one base. Its accessor is at `0x004c6b28`. No recovered site throws it, so no
 * constructor is reconstructed.
 */
class AttributeError : public StandardError {};

} // namespace Py
