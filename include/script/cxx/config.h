#pragma once

#include <Python.h>

/**
 * @file
 *
 * Configuration header of the PyCXX binding layer.
 *
 * PyCXX is the C++ wrapper the game compiled into its own image around the embedded CPython 2.0
 * interpreter. Twenty-nine of its classes survive as RTTI descriptors, and the binding is
 * reconstructed here because the game built it rather than linking it, while the interpreter
 * underneath is vendored upstream and is recorded in `src/python/MANIFEST.md` instead.
 *
 * Upstream PyCXX splits into `CXX/Config.hxx`, `CXX/Objects.hxx`, `CXX/Exception.hxx`, and
 * `CXX/Extensions.hxx`. This tree gives every class its own header under `script/cxx/`, and this
 * file takes the part of `Config.hxx` that survives, which is the interpreter include.
 *
 * Two differences from released PyCXX are established by the image rather than assumed. The port
 * replaced `std::string` with the game's own HxStr, which every recovered PyCXX signature that
 * takes a string uses. The port also has no `owned` flag on Py::Object, and it adopts a new
 * reference through the separate Py::FromAPI holder instead.
 *
 * Member names under `script/cxx/` follow PyCXX's own lower-case style, `accepts`, `validate`,
 * `setItem`, and `new_reference_to` among them, rather than the CamelCase the rest of this tree
 * uses. The names are third-party API and are preserved. Data members are the exception, because
 * no member name survives compilation, so those take the tree's `m` prefix.
 *
 * The interpreter headers are vendored outside this tree, at
 * `.wiswa-ci/freq/Python-2.0/Include`, so a syntax check of anything under `script/cxx/` needs
 * that directory and the port's own `src/python/PC` on the include path.
 */
