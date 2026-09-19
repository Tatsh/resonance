#pragma once

#include "script/cxx/standarderror.h"

namespace Py {

/**
 * Exception for an unresolved name, mirroring Python's NameError.
 *
 * `Q22Py9NameError` in the RTTI descriptor at `0x008ef390`, with Py::StandardError at offset 0 as
 * its one base. Its accessor is at `0x005ae670`. No recovered site throws it, so no constructor is
 * reconstructed.
 */
class NameError : public StandardError {};

} // namespace Py
