# Writes the build identification header that patched builds show on the title screen. Runs as a
# script on every build, so the revision and the time are those of the build rather than of the
# last configure. Inputs: SOURCE_DIR, the repository, and OUTPUT, the header to write.

# A release tag on the commit identifies the build, always shown with a leading v. Any other commit
# is identified by its hash.
execute_process(
  COMMAND git describe --tags --exact-match --match "v[0-9]*" --match "[0-9]*" HEAD
  WORKING_DIRECTORY "${SOURCE_DIR}"
  OUTPUT_VARIABLE _revision
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  RESULT_VARIABLE _result)
if(_result EQUAL 0 AND NOT _revision STREQUAL "")
  string(REGEX REPLACE "^v" "" _revision "${_revision}")
  set(_revision "v${_revision}")
else()
  execute_process(
    COMMAND git rev-parse --short=7 HEAD
    WORKING_DIRECTORY "${SOURCE_DIR}"
    OUTPUT_VARIABLE _revision
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE _result)
endif()
if(NOT _result EQUAL 0 OR _revision STREQUAL "")
  set(_revision "unknown")
endif()
string(TIMESTAMP _time "%Y-%m-%d %H:%M")

set(_content "#pragma once\n\n")
string(APPEND _content "#define RESONANCE_GIT_REVISION \"${_revision}\"\n")
string(APPEND _content "#define RESONANCE_BUILD_TIME \"${_time}\"\n")

# Rewriting an unchanged header would recompile the file that includes it on every build.
if(EXISTS "${OUTPUT}")
  file(READ "${OUTPUT}" _old)
endif()
if(NOT _old STREQUAL _content)
  file(WRITE "${OUTPUT}" "${_content}")
endif()
