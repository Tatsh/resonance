set(RESONANCE_GFX_SRCS gfxdevice.cpp gsdoublebuffer.cpp vramtable.cpp)

# The VU microprograms and the DMA chains that upload them. The DVP assembler builds them. It ships
# with the SDK beside the Emotion Engine compiler.
find_program(DVP_AS dvp-as REQUIRED HINTS "$ENV{PS2DEV}/dvp/bin")
set(RESONANCE_GFX_VU_SRCS vu0microcode.vsm vu1microcode.vsm)
set(RESONANCE_GFX_VU_OBJS)
foreach(VSM ${RESONANCE_GFX_VU_SRCS})
  get_filename_component(VSM_NAME ${VSM} NAME_WE)
  set(VSM_OBJ "${CMAKE_CURRENT_BINARY_DIR}/${VSM_NAME}.vsm.o")
  add_custom_command(
    OUTPUT ${VSM_OBJ}
    COMMAND ${DVP_AS} -o ${VSM_OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${VSM}
    DEPENDS ${VSM}
    COMMENT "Assembling ${VSM}"
    VERBATIM)
  list(APPEND RESONANCE_GFX_VU_OBJS ${VSM_OBJ})
endforeach()
set_source_files_properties(${RESONANCE_GFX_VU_OBJS} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)

add_library(resonance_gfx STATIC ${RESONANCE_GFX_SRCS} ${RESONANCE_GFX_VU_OBJS})
target_link_libraries(resonance_gfx PRIVATE resonance_common)
