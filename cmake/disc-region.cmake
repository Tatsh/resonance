# Checks that the original disc matches VIDEO_STANDARD. A PAL build requires the European disc
# and an NTSC build the North American one.

set(RESONANCE_DISC_IMAGE
    ""
    CACHE PATH "Original FreQuency disc image (cue, bin, or ISO), or the disc root directory, \
that the image target rebuilds. It must be the release VIDEO_STANDARD selects.")

if(RESONANCE_DISC_IMAGE AND NOT BUILD_DOCS_ONLY)
  execute_process(
    COMMAND "${RESONANCE_TOOL_BUILD_IMAGE}" --identify "${RESONANCE_DISC_IMAGE}"
    OUTPUT_VARIABLE _resonance_disc_executable
    ERROR_VARIABLE _resonance_disc_error
    RESULT_VARIABLE _resonance_disc_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT _resonance_disc_result EQUAL 0)
    message(FATAL_ERROR "Cannot identify ${RESONANCE_DISC_IMAGE}:\n${_resonance_disc_error}")
  endif()
  if(_resonance_disc_executable STREQUAL "SCES_507.91")
    set(_resonance_disc_standard PAL)
  else()
    set(_resonance_disc_standard NTSC)
  endif()
  if(NOT _resonance_disc_standard STREQUAL VIDEO_STANDARD)
    message(FATAL_ERROR "VIDEO_STANDARD is ${VIDEO_STANDARD}, and the original boots "
                        "${_resonance_disc_executable}, the ${_resonance_disc_standard} release.")
  endif()
endif()
