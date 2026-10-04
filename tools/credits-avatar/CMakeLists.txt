add_executable(credits-avatar avatar.cpp creditsheader.cpp main.cpp)
target_link_libraries(credits-avatar PRIVATE argparse::argparse resonance_tools_common
                                             resonance_tools_https spdlog::spdlog)
