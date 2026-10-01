# Toolchain file for the Emotion Engine. It selects the cross compiler and nothing else from the
# ps2dev distribution. The Sony libraries, their headers, the start-up code, and the link script
# are reconstructed in this tree. No SDK include or library directory is added here.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR mips)

if(DEFINED ENV{PS2DEV})
  set(PS2DEV
      "$ENV{PS2DEV}"
      CACHE PATH "ps2dev distribution root that provides the cross compiler.")
endif()
find_program(
  CMAKE_C_COMPILER mips64r5900el-ps2-elf-gcc
  PATHS "${PS2DEV}/ee/bin"
  REQUIRED)
find_program(
  CMAKE_CXX_COMPILER mips64r5900el-ps2-elf-g++
  PATHS "${PS2DEV}/ee/bin"
  REQUIRED)

# The original compiler loaded through a null pointer where the code did and retained the null
# tests that follow such a load. Path isolation would replace the loads with a trap, and null-check
# deletion would drop the tests. It also let signed arithmetic wrap, and overflow tests such as
# CPython's size checks in tuplerepeat remain in the code.
set(_ee_flags "-D_EE -O2 -G0 -fno-isolate-erroneous-paths-dereference")
string(APPEND _ee_flags " -fno-delete-null-pointer-checks -fwrapv")
set(CMAKE_C_FLAGS_INIT "${_ee_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_ee_flags}")
set(CMAKE_ASM_FLAGS_INIT "-D_EE -G0")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
