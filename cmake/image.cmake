# Defines the image target, which writes a bootable FreQuency CD image, a raw MODE2/2352 bin and
# its cue sheet, with the built executable and EZMIDI.IRX in place of the originals.
#
# scripts/build-image.py rebuilds the image from an original disc image, so the target exists only
# once RESONANCE_DISC_IMAGE names one (cue, bin, or ISO). The script needs requests. Point
# Python3_EXECUTABLE at an interpreter that has it when the default one does not.

set(RESONANCE_DISC_IMAGE
    ""
    CACHE FILEPATH "Original FreQuency disc image (cue, bin, or ISO) the image target rebuilds.")
set(RESONANCE_IMAGE_OUTPUT
    "${CMAKE_BINARY_DIR}/resonance.cue"
    CACHE FILEPATH "Cue sheet the image target writes. The bin is written beside it.")

if(NOT RESONANCE_DISC_IMAGE)
  message(STATUS "Set RESONANCE_DISC_IMAGE to an original disc image to enable the image target.")
  return()
endif()
if(NOT TARGET ezmidi_irx)
  message(STATUS "The image target needs the IOP toolchain for EZMIDI.IRX, and it is disabled.")
  return()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

get_filename_component(_resonance_image_dir "${RESONANCE_IMAGE_OUTPUT}" DIRECTORY)
get_filename_component(_resonance_image_stem "${RESONANCE_IMAGE_OUTPUT}" NAME_WLE)
set(_resonance_irx "$<TARGET_PROPERTY:ezmidi_irx,IRX_FILE>")
add_custom_command(
  OUTPUT "${RESONANCE_IMAGE_OUTPUT}" "${_resonance_image_dir}/${_resonance_image_stem}.bin"
  COMMAND
    Python3::Interpreter "${CMAKE_SOURCE_DIR}/scripts/build-image.py" "${RESONANCE_DISC_IMAGE}"
    "${RESONANCE_IMAGE_OUTPUT}" --overwrite --resonance-bin $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    --ezmidi-irx "${_resonance_irx}"
  DEPENDS ${CMAKE_PROJECT_NAME} ezmidi_irx "${_resonance_irx}" "${RESONANCE_DISC_IMAGE}"
          "${CMAKE_SOURCE_DIR}/scripts/build-image.py"
  COMMENT "Writing ${RESONANCE_IMAGE_OUTPUT}"
  VERBATIM)
add_custom_target(image DEPENDS "${RESONANCE_IMAGE_OUTPUT}")
