#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"
#include "script/cxx/seqbase.h"

namespace Py {

/**
 * Handle on a Python list.
 *
 * `Q22Py4List` in the RTTI descriptor at `0x00902180`, with `Py::SeqBase<Py::Object>` at offset 0
 * as its one base.
 *
 * Nothing beyond the descriptor is recovered. Its vtable was not located, and the harvest keyed
 * its accessor to `0x005ae430`, which is the shared base accessor rather than this class's own, so
 * the accessor address is not established either. No constructor, no override, and no member is
 * reconstructed, and the declaration exists to record the class and its base.
 */
class List : public SeqBase<Object> {};

} // namespace Py
