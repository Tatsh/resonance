#pragma once

#include <expected>
#include <string>
#include <utility>

#include "error.h"

/**
 * Report that the disc image cannot be read or rewritten.
 *
 * @param message Sentence describing the failure.
 * @return An `ErrorCode::InvalidInput` error.
 */
inline std::unexpected<Error> discImageError(std::string message) {
    return std::unexpected(Error{ErrorCode::InvalidInput, std::move(message)});
}

/**
 * Report that build artifacts cannot be fetched from GitHub.
 *
 * @param message Sentence describing the failure.
 * @return An `ErrorCode::Network` error.
 */
inline std::unexpected<Error> artifactError(std::string message) {
    return std::unexpected(Error{ErrorCode::Network, std::move(message)});
}

/**
 * Report that a file cannot be read or written.
 *
 * @param message Sentence describing the failure.
 * @return An `ErrorCode::Io` error.
 */
inline std::unexpected<Error> ioError(std::string message) {
    return std::unexpected(Error{ErrorCode::Io, std::move(message)});
}
