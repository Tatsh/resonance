# Writes the build identification header that patched builds show on the title screen. Runs as a
# script on every build, so the commit and the time are those of the build rather than of the last
# configure. Inputs: SOURCE_DIR, the repository, and OUTPUT, the header to write.

execute_process(
  COMMAND git rev-parse --short=7 HEAD
  WORKING_DIRECTORY "${SOURCE_DIR}"
  OUTPUT_VARIABLE _sha
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  RESULT_VARIABLE _result)
if(NOT _result EQUAL 0 OR _sha STREQUAL "")
  set(_sha "unknown")
endif()
string(TIMESTAMP _time "%Y-%m-%d %H:%M")

set(_content "#pragma once\n\n")
string(APPEND _content "#define RESONANCE_GIT_SHA \"${_sha}\"\n")
string(APPEND _content "#define RESONANCE_BUILD_TIME \"${_time}\"\n")

# Rewriting an unchanged header would recompile the file that includes it on every build.
if(EXISTS "${OUTPUT}")
  file(READ "${OUTPUT}" _old)
endif()
if(NOT _old STREQUAL _content)
  file(WRITE "${OUTPUT}" "${_content}")
endif()
