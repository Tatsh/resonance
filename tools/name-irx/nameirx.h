#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <vector>

#include "error.h"

/**
 * Return the IRX with the module name written into its `.iopmod` header.
 *
 * The name comes from the `ModuleInfo` record that the header points to. The rest of the file
 * moves by the growth of the header, rounded to the alignment of the loadable segment, and the ELF,
 * program, and section headers are adjusted to match.
 *
 * @param data The IRX file.
 * @return The rewritten file, or the input when the header already has a name. An
 * `ErrorCode::InvalidInput` error when the file is not an IRX or its layout cannot be rewritten.
 */
std::expected<std::vector<std::uint8_t>, Error> nameIrx(std::span<const std::uint8_t> data);
