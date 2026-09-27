set(RESONANCE_NETFLOW_SRCS netflow.cpp)

add_library(resonance_netflow STATIC ${RESONANCE_NETFLOW_SRCS})
target_link_libraries(resonance_netflow PRIVATE resonance_common)
