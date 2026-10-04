#include <argparse/argparse.hpp>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>

#include "fileio.h"
#include "logging.h"
#include "nameirx.h"

namespace {

constexpr int kUsageErrorStatus = 2;

} // namespace

int main(int argc, char *argv[]) {
    argparse::ArgumentParser parser("name-irx", "", argparse::default_arguments::help);
    parser.add_description("Write an IRX module's name into its .iopmod header, in place.");
    parser.add_argument("irx").help("The IRX file to rewrite.");
    parser.add_argument("--debug").help("Enable debug logging.").flag();
    // argparse reports usage errors only by throwing.
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << parser.usage() << "\nname-irx: error: " << e.what() << '\n';
        return kUsageErrorStatus;
    }
    setupLogging(parser.get<bool>("--debug"));
    const std::filesystem::path irx = parser.get<std::string>("irx");
    const auto named = readFile(irx).and_then([&irx](const auto &data) {
        return nameIrx(data).and_then([&](const auto &result) -> std::expected<void, Error> {
            return result == data ? std::expected<void, Error>() : writeFile(irx, result);
        });
    });
    if (!named) {
        spdlog::error("Naming the module failed. {}", named.error().message);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
