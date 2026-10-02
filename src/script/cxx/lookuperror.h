#pragma once

#include "script/cxx/standarderror.h"

namespace Py {

/**
 * Intermediate of the PyCXX exception hierarchy, mirroring Python's LookupError.
 *
 * Its RTTI descriptor is at `0x008efe90`. It has Py::StandardError at offset 0 as its one base. Its
 * accessor is at `0x004c7e50`. Py::KeyError derives from it. Nothing in the image throws it
 * directly.
 */
class LookupError : public StandardError {};

} // namespace Py
