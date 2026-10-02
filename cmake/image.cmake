# Defines the image target, which writes a bootable FreQuency CD image, a raw MODE2/2352 bin and
# its cue sheet, with the built executable and EZMIDI.IRX in place of the originals.
#
# scripts/build-image.py rebuilds the image from an original disc image, so the target exists only
# once RESONANCE_DISC_IMAGE specifies one (cue, bin, or ISO) or the disc root directory.
# The script needs requests. Point Python3_EXECUTABLE at an interpreter that has it when the
# default one does not.

set(RESONANCE_DISC_IMAGE
    ""
    CACHE PATH "Original FreQuency disc image (cue, bin, or ISO), or the disc root directory, \
that the image target rebuilds.")
set(RESONANCE_DISC_SYSTEM_AREA
    ""
    CACHE FILEPATH "First 12 sectors (the boot logo) of the original disc, for a disc root \
directory. The sectors are zero when unset.")
set(RESONANCE_IMAGE_OUTPUT
    "${CMAKE_BINARY_DIR}/resonance.cue"
    CACHE FILEPATH "Cue sheet the image target writes. The bin is written beside it.")
# The console loads only the program segment, so the debug information is dead weight on the disc.
# The stripped executable fits the original's extent and is written in place.
option(RESONANCE_IMAGE_STRIP "Put an executable without debug information on the image." ON)

if(NOT RESONANCE_DISC_IMAGE)
  message(STATUS "Set RESONANCE_DISC_IMAGE to an original disc image to enable the image target.")
  return()
endif()
if(NOT TARGET ezmidi_irx)
  message(STATUS "The image target needs the IOP toolchain for EZMIDI.IRX, and it is disabled.")
  return()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# A disc root is rebuilt whenever one of its files changes.
if(IS_DIRECTORY "${RESONANCE_DISC_IMAGE}")
  file(GLOB_RECURSE _resonance_disc_inputs CONFIGURE_DEPENDS LIST_DIRECTORIES false
       "${RESONANCE_DISC_IMAGE}/*")
else()
  set(_resonance_disc_inputs "${RESONANCE_DISC_IMAGE}")
endif()
set(_resonance_system_area_args)
if(RESONANCE_DISC_SYSTEM_AREA)
  set(_resonance_system_area_args --system-area "${RESONANCE_DISC_SYSTEM_AREA}")
  list(APPEND _resonance_disc_inputs "${RESONANCE_DISC_SYSTEM_AREA}")
endif()

get_filename_component(_resonance_image_dir "${RESONANCE_IMAGE_OUTPUT}" DIRECTORY)
get_filename_component(_resonance_image_stem "${RESONANCE_IMAGE_OUTPUT}" NAME_WLE)
set(_resonance_irx "$<TARGET_PROPERTY:ezmidi_irx,IRX_FILE>")
set(_resonance_executable "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>")
if(RESONANCE_IMAGE_STRIP)
  set(_resonance_executable "${CMAKE_BINARY_DIR}/RESONANCE.ELF")
  add_custom_command(
    OUTPUT "${_resonance_executable}"
    COMMAND "${CMAKE_OBJCOPY}" --strip-debug $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            "${_resonance_executable}"
    DEPENDS ${CMAKE_PROJECT_NAME}
    COMMENT "Writing ${_resonance_executable}"
    VERBATIM)
endif()
add_custom_command(
  OUTPUT "${RESONANCE_IMAGE_OUTPUT}" "${_resonance_image_dir}/${_resonance_image_stem}.bin"
  COMMAND
    Python3::Interpreter "${CMAKE_SOURCE_DIR}/scripts/build-image.py" "${RESONANCE_DISC_IMAGE}"
    "${RESONANCE_IMAGE_OUTPUT}" --overwrite --resonance-bin "${_resonance_executable}"
    --ezmidi-irx "${_resonance_irx}" ${_resonance_system_area_args}
  DEPENDS ${CMAKE_PROJECT_NAME} ezmidi_irx "${_resonance_executable}" "${_resonance_irx}"
          ${_resonance_disc_inputs} "${CMAKE_SOURCE_DIR}/scripts/build-image.py"
  COMMENT "Writing ${RESONANCE_IMAGE_OUTPUT}"
  VERBATIM)
add_custom_target(image DEPENDS "${RESONANCE_IMAGE_OUTPUT}")
