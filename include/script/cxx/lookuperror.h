#pragma once

#include "script/cxx/standarderror.h"

namespace Py {

/**
 * Intermediate of the PyCXX exception hierarchy, mirroring Python's LookupError.
 *
 * `Q22Py11LookupError` in the RTTI descriptor at `0x008efe90`, with Py::StandardError at offset 0
 * as its one base. Its accessor is at `0x004c7e50`. Py::KeyError derives from it. Nothing in the
 * image throws it directly.
 */
class LookupError : public StandardError {};

} // namespace Py
