#pragma once

#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on a Python float.
 *
 * `Q22Py5Float` in the RTTI descriptor at `0x00902170`, with Py::Object at offset 0 as its one
 * base. Its accessor is at `0x00454290` and its destructor at `0x00454218`.
 *
 * Its vtable was not located, so no accepts() override is reconstructed. Every sibling of this
 * class in the hierarchy narrows accepts(), and a float wrapper that accepted anything would be
 * surprising, but the table is what proves an override and the table is missing. The declaration
 * therefore records the class and its base and nothing more.
 */
class Float : public Object {};

} // namespace Py
