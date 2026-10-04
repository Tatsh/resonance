add_executable(name-irx main.cpp nameirx.cpp)
target_link_libraries(name-irx PRIVATE argparse::argparse elfio::elfio resonance_tools_common
                                       spdlog::spdlog)
