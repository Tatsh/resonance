# Defines the iso target, which writes a bootable FreQuency disc image with the built executable
# and EZMIDI.IRX in place of the originals.
#
# scripts/build-iso.py rewrites a copy of an original disc image, so the target exists only once
# RESONANCE_DISC_IMAGE names one (cue, bin, or ISO). The script needs click and requests. Point
# Python3_EXECUTABLE at an interpreter that has them when the default one does not.

set(RESONANCE_DISC_IMAGE
    ""
    CACHE FILEPATH "Original FreQuency disc image (cue, bin, or ISO) the iso target rewrites.")
set(RESONANCE_ISO_OUTPUT
    "${CMAKE_BINARY_DIR}/resonance.iso"
    CACHE FILEPATH "Disc image the iso target writes.")

if(NOT RESONANCE_DISC_IMAGE)
  message(STATUS "Set RESONANCE_DISC_IMAGE to an original disc image to enable the iso target.")
  return()
endif()
if(NOT TARGET ezmidi_irx)
  message(STATUS "The iso target needs the IOP toolchain for EZMIDI.IRX, and it is disabled.")
  return()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(_resonance_irx "$<TARGET_PROPERTY:ezmidi_irx,IRX_FILE>")
add_custom_command(
  OUTPUT "${RESONANCE_ISO_OUTPUT}"
  COMMAND
    Python3::Interpreter "${CMAKE_SOURCE_DIR}/scripts/build-iso.py" "${RESONANCE_DISC_IMAGE}"
    "${RESONANCE_ISO_OUTPUT}" --overwrite --resonance-bin $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    --ezmidi-irx "${_resonance_irx}"
  DEPENDS ${CMAKE_PROJECT_NAME} ezmidi_irx "${_resonance_irx}" "${RESONANCE_DISC_IMAGE}"
          "${CMAKE_SOURCE_DIR}/scripts/build-iso.py"
  COMMENT "Writing ${RESONANCE_ISO_OUTPUT}"
  VERBATIM)
add_custom_target(iso DEPENDS "${RESONANCE_ISO_OUTPUT}")
