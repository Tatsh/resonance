# Links EZMIDI.IRX for the IOP.
#
# The module is R3000 code the IOP loader relocates. The file drives the IOP toolchain directly
# with custom commands: compile, relocatable link, strip, then fix up into an IRX. The module
# builds against the reconstructed IOP headers under sce/iop and imports through its stub tables.
# Without the IOP toolchain, configuration records the gap and defines no target.

if(TARGET ezmidi_irx)
  return()
endif()

find_program(EZMIDI_IOP_CC mipsel-none-elf-gcc PATHS "$ENV{PS2DEV}/iop/bin" "$ENV{PS2DEV}/bin")
find_program(EZMIDI_IOP_STRIP mipsel-none-elf-strip PATHS "$ENV{PS2DEV}/iop/bin" "$ENV{PS2DEV}/bin")
find_program(EZMIDI_IOP_FIXUP NAMES srxfixup iopfixup PATHS "$ENV{PS2DEV}/bin")

set(EZMIDI_MISSING "")
foreach(_probe EZMIDI_IOP_CC EZMIDI_IOP_STRIP EZMIDI_IOP_FIXUP)
  if(NOT ${_probe})
    list(APPEND EZMIDI_MISSING ${_probe})
  endif()
endforeach()
if(EZMIDI_MISSING)
  message(STATUS "EZMIDI.IRX skipped, missing: ${EZMIDI_MISSING}")
  return()
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(EZMIDI_SRC_DIR "${CMAKE_SOURCE_DIR}/src/ezmidi")
set(EZMIDI_BUILD_DIR "${CMAKE_BINARY_DIR}/ezmidi")
set(EZMIDI_LINKFILE "${CMAKE_SOURCE_DIR}/sce/iop/iop.ld")
file(MAKE_DIRECTORY "${EZMIDI_BUILD_DIR}")

file(GLOB EZMIDI_HEADERS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/include/ezmidi/*.h"
     "${CMAKE_SOURCE_DIR}/sce/iop/include/*.h")

set(EZMIDI_IOP_FLAGS
    -G0
    -Os
    -Wall
    -Wextra
    -fno-builtin
    -msoft-float
    -mno-explicit-relocs
    -I${CMAKE_SOURCE_DIR}/sce/iop/include
    -I${CMAKE_SOURCE_DIR}/include)

# The units follow the shipped module: the entry first, the RPC server, the synthesiser, and the
# import stubs last.
set(EZMIDI_SRCS midi_ent.c midi_com.c midi_hsyn.c ilb_stub.s)
set(EZMIDI_OBJS "")
foreach(_src ${EZMIDI_SRCS})
  get_filename_component(_base "${_src}" NAME_WE)
  set(_obj "${EZMIDI_BUILD_DIR}/${_base}.o")
  add_custom_command(
    OUTPUT "${_obj}"
    COMMAND ${EZMIDI_IOP_CC} ${EZMIDI_IOP_FLAGS} -c "${EZMIDI_SRC_DIR}/${_src}" -o "${_obj}"
    DEPENDS "${EZMIDI_SRC_DIR}/${_src}" ${EZMIDI_HEADERS}
    COMMENT "Compiling ${_src} for the IOP"
    VERBATIM)
  list(APPEND EZMIDI_OBJS "${_obj}")
endforeach()

# The shipped module retains its symbol table, so the built one does too unless this is set.
option(EZMIDI_IRX_STRIP "Remove the symbols EZMIDI.IRX does not need." OFF)
# srxfixup writes a module without symbols through -o and one with them through -r.
set(_ezmidi_fixup_input "${EZMIDI_BUILD_DIR}/EZMIDI.elf")
set(_ezmidi_fixup_output -r)
set(_ezmidi_strip_command "")
if(EZMIDI_IRX_STRIP)
  set(_ezmidi_fixup_input "${EZMIDI_BUILD_DIR}/EZMIDI.stripped.elf")
  set(_ezmidi_fixup_output -o)
  set(_ezmidi_strip_command COMMAND ${EZMIDI_IOP_STRIP} --strip-unneeded -o
                            "${_ezmidi_fixup_input}" "${EZMIDI_BUILD_DIR}/EZMIDI.elf")
endif()

add_custom_command(
  OUTPUT "${EZMIDI_BUILD_DIR}/EZMIDI.IRX"
  COMMAND ${EZMIDI_IOP_CC} -T${EZMIDI_LINKFILE} -nostdlib -o "${EZMIDI_BUILD_DIR}/EZMIDI.elf"
          ${EZMIDI_OBJS} -Wl,-r -Wl,-dc
  ${_ezmidi_strip_command}
  # The shipped module starts .text with its entry point, as the built module does.
  COMMAND ${EZMIDI_IOP_FIXUP} --rb --irx1 --allow-zero-text ${_ezmidi_fixup_output}
          "${EZMIDI_BUILD_DIR}/EZMIDI.IRX" "${_ezmidi_fixup_input}"
  # srxfixup leaves the name in the .iopmod header empty. The shipped module has it there.
  COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/scripts/name-irx.py"
          "${EZMIDI_BUILD_DIR}/EZMIDI.IRX"
  DEPENDS ${EZMIDI_OBJS} "${EZMIDI_LINKFILE}" "${CMAKE_SOURCE_DIR}/scripts/name-irx.py"
  COMMENT "Linking EZMIDI.IRX"
  VERBATIM)

add_custom_target(ezmidi_irx ALL DEPENDS "${EZMIDI_BUILD_DIR}/EZMIDI.IRX")
# The disc image target reads the module path from here.
set_property(TARGET ezmidi_irx PROPERTY IRX_FILE "${EZMIDI_BUILD_DIR}/EZMIDI.IRX")
