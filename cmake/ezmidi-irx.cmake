# Links EZMIDI.IRX for the IOP.
#
# The archive in this directory targets the Emotion Engine, while the server
# module needs R3000 code, IOP headers, and import stubs resolved by the
# loader. This module therefore drives the IOP toolchain directly with custom
# commands. The steps follow the SDK sample for relocatable modules: compile,
# partial link, strip, then fix up into EZMIDI.IRX.
#
# Without the IOP toolchain or the SDK IOP tree, configuration records the
# gap and defines no target. The main build never depends on the target
# defined here.

if(TARGET ezmidi_irx)
  return()
endif()

find_program(EZMIDI_IOP_CC mipsel-none-elf-gcc PATHS "$ENV{PS2DEV}/bin")
find_program(EZMIDI_IOP_STRIP mipsel-none-elf-strip PATHS "$ENV{PS2DEV}/bin")
find_program(EZMIDI_IOP_FIXUP NAMES iopfixup srxfixup PATHS "$ENV{PS2DEV}/bin")

set(EZMIDI_PS2SDK "$ENV{PS2SDK}")
set(EZMIDI_IOP_INCLUDE "${EZMIDI_PS2SDK}/iop/include")
set(EZMIDI_COMMON_INCLUDE "${EZMIDI_PS2SDK}/common/include")
set(EZMIDI_LINKFILE "${EZMIDI_PS2SDK}/iop/startup/linkfile")

set(EZMIDI_MISSING "")
foreach(_probe EZMIDI_IOP_CC EZMIDI_IOP_STRIP EZMIDI_IOP_FIXUP)
  if(NOT ${_probe})
    list(APPEND EZMIDI_MISSING ${_probe})
  endif()
endforeach()
foreach(_dir EZMIDI_IOP_INCLUDE EZMIDI_COMMON_INCLUDE)
  if(NOT IS_DIRECTORY "${${_dir}}")
    list(APPEND EZMIDI_MISSING ${_dir})
  endif()
endforeach()
if(NOT EXISTS "${EZMIDI_LINKFILE}")
  list(APPEND EZMIDI_MISSING EZMIDI_LINKFILE)
endif()

if(EZMIDI_MISSING)
  message(STATUS "EZMIDI.IRX skipped, missing: ${EZMIDI_MISSING}")
  return()
endif()

set(EZMIDI_IRX_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(EZMIDI_IRX_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/irx")
file(MAKE_DIRECTORY "${EZMIDI_IRX_BUILD_DIR}")

set(EZMIDI_HEADERS
    "${CMAKE_SOURCE_DIR}/include/ezmidi/common.h"
    "${CMAKE_SOURCE_DIR}/include/ezmidi/ezmidi.h"
    "${CMAKE_SOURCE_DIR}/include/ezmidi/imports.h"
    "${CMAKE_SOURCE_DIR}/include/ezmidi/synth.h"
    "${EZMIDI_IRX_DIR}/irx_imports.h")

set(EZMIDI_IOP_FLAGS
    -D_IOP
    -fno-builtin
    -G0
    -Os
    -Wall
    -msoft-float
    -mno-explicit-relocs
    -I${EZMIDI_IOP_INCLUDE}
    -I${EZMIDI_COMMON_INCLUDE}
    -I${EZMIDI_IRX_DIR}
    -I${CMAKE_SOURCE_DIR}/include)

# midi_ent.o comes first so the module entry and handlers lead .text. The fixup
# resolves the entry from the start symbol wherever the compiler places it.
set(EZMIDI_SRCS midi_ent.c key.c midi_com.c midi_hsyn.c notes.c irx_id.c)
set(EZMIDI_OBJS "")
foreach(_src ${EZMIDI_SRCS})
  get_filename_component(_base "${_src}" NAME_WE)
  set(_obj "${EZMIDI_IRX_BUILD_DIR}/${_base}.o")
  add_custom_command(
    OUTPUT "${_obj}"
    COMMAND ${EZMIDI_IOP_CC} ${EZMIDI_IOP_FLAGS} -c "${EZMIDI_IRX_DIR}/${_src}" -o "${_obj}"
    DEPENDS "${EZMIDI_IRX_DIR}/${_src}" ${EZMIDI_HEADERS}
    COMMENT "Compiling ${_src} for the IOP"
    VERBATIM)
  list(APPEND EZMIDI_OBJS "${_obj}")
endforeach()

file(READ "${EZMIDI_IRX_DIR}/imports.lst" EZMIDI_IMPORTS_LST)
file(WRITE "${EZMIDI_IRX_BUILD_DIR}/build-imports.c"
     "#include \"irx_imports.h\"\n${EZMIDI_IMPORTS_LST}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${EZMIDI_IRX_DIR}/imports.lst")

add_custom_command(
  OUTPUT "${EZMIDI_IRX_BUILD_DIR}/build-imports.o"
  COMMAND ${EZMIDI_IOP_CC} ${EZMIDI_IOP_FLAGS} -fno-toplevel-reorder -c
          "${EZMIDI_IRX_BUILD_DIR}/build-imports.c" -o "${EZMIDI_IRX_BUILD_DIR}/build-imports.o"
  DEPENDS "${EZMIDI_IRX_BUILD_DIR}/build-imports.c" "${EZMIDI_IRX_DIR}/irx_imports.h"
          ${EZMIDI_HEADERS}
  COMMENT "Compiling the import table for the IOP"
  VERBATIM)

add_custom_command(
  OUTPUT "${EZMIDI_IRX_BUILD_DIR}/EZMIDI.IRX"
  COMMAND ${EZMIDI_IOP_CC} -T${EZMIDI_LINKFILE} -Os -o "${EZMIDI_IRX_BUILD_DIR}/EZMIDI.elf"
          ${EZMIDI_OBJS} "${EZMIDI_IRX_BUILD_DIR}/build-imports.o" -nostdlib -dc -r
  COMMAND ${EZMIDI_IOP_STRIP} --strip-unneeded --remove-section=.pdr --remove-section=.comment
          --remove-section=.mdebug.abi32 --remove-section=.gnu.attributes -o
          "${EZMIDI_IRX_BUILD_DIR}/EZMIDI.stripped.elf" "${EZMIDI_IRX_BUILD_DIR}/EZMIDI.elf"
  COMMAND ${EZMIDI_IOP_FIXUP} --rb --irx1 --allow-zero-text -o
          "${EZMIDI_IRX_BUILD_DIR}/EZMIDI.IRX" "${EZMIDI_IRX_BUILD_DIR}/EZMIDI.stripped.elf"
  DEPENDS ${EZMIDI_OBJS} "${EZMIDI_IRX_BUILD_DIR}/build-imports.o"
  COMMENT "Linking EZMIDI.IRX"
  VERBATIM)

add_custom_target(ezmidi_irx DEPENDS "${EZMIDI_IRX_BUILD_DIR}/EZMIDI.IRX")
# The disc image target reads the module path from here.
set_property(TARGET ezmidi_irx PROPERTY IRX_FILE "${EZMIDI_IRX_BUILD_DIR}/EZMIDI.IRX")
