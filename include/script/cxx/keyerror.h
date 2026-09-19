#pragma once

#include "script/cxx/lookuperror.h"

namespace Py {

/**
 * Exception for a missing mapping key, mirroring Python's KeyError.
 *
 * `Q22Py8KeyError` in the RTTI descriptor at `0x008eece8`, with Py::LookupError at offset 0 as its
 * one base. Its accessor is at `0x004c7d88`, an address the descriptor harvest mis-attributed to
 * `RndText`.
 *
 * No recovered site throws it, so no constructor is reconstructed. The descriptor exists because
 * one translation unit mentions the type in a throw or a catch.
 */
class KeyError : public LookupError {};

} // namespace Py
