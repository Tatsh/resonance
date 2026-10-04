#pragma once

#include <string>

/** Host tools that the build runs. */
namespace Tools {

/** Kind of failure a tool reports. */
enum class ErrorCode {
    InvalidInput, /*!< An input file does not have the expected format. */
    Io,           /*!< Reading or writing a file failed. */
    Network,      /*!< A download failed. */
    Process,      /*!< A child process failed. */
};

/** Failure returned through `std::expected`. */
struct Error {
    ErrorCode code;      /*!< Kind of failure. */
    std::string message; /*!< Sentence describing the failure, for the log. */
};

} // namespace Tools
