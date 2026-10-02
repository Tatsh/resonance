#pragma once

#include "script/cxx/exception.h"

namespace Py {

/**
 * Intermediate of the PyCXX exception hierarchy, mirroring Python's StandardError.
 *
 * Its RTTI descriptor is at `0x008eef28`. It has Py::Exception at offset 0 as its one base. Six
 * descriptors derive from it. The image never throws it directly and it has no members. The
 * abstract intermediate that released PyCXX declares matches both facts.
 */
class StandardError : public Exception {};

} // namespace Py
