set(RESONANCE_MATH_SRCS
    box.cpp
    color.cpp
    frustum.cpp
    plane.cpp
    quaternion.cpp
    sphere.cpp
    transformops.cpp
    vector2.cpp
    vector3.cpp)

add_library(resonance_math STATIC ${RESONANCE_MATH_SRCS})
target_link_libraries(resonance_math PRIVATE resonance_common)
