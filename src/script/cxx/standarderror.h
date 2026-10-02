#pragma once

#include "script/cxx/exception.h"

namespace Py {

/**
 * Intermediate of the PyCXX exception hierarchy, mirroring Python's StandardError.
 *
 * `Q22Py13StandardError` in the RTTI descriptor at `0x008eef28`, with Py::Exception at offset 0 as
 * its one base. Six descriptors derive from it. Nothing in the image throws it directly and it has
 * no members, which matches the abstract intermediate released PyCXX declares.
 */
class StandardError : public Exception {};

} // namespace Py
