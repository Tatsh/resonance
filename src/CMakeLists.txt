include_directories(. ${CMAKE_CURRENT_BINARY_DIR}/generated ${CMAKE_CURRENT_BINARY_DIR})

if(CMAKE_BUILD_TYPE STREQUAL Debug AND (CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID
                                                                              STREQUAL Clang))
  set(DEBUG_FLAGS -ggdb -Wall -Werror -Wextra -Wno-unknown-pragmas)
endif()

set(LIBNAME_SRCS other.h)
add_library(name STATIC ${LIBNAME_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/generated/namedebug.cpp)
target_compile_options(name PRIVATE ${DEBUG_FLAGS})
target_link_libraries(name PRIVATE ${SHARED_DEPS})

add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE ${DEBUG_FLAGS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE name ${SHARED_DEPS})
