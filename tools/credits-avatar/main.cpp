#include <argparse/argparse.hpp>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>

#include "avatar.h"
#include "creditsheader.h"
#include "fileio.h"
#include "logging.h"

namespace {

constexpr int kUsageErrorStatus = 2;
constexpr int kDefaultSize = 128;
constexpr std::string_view kDefaultMagick = "magick";
constexpr std::string_view kEscapedNewline = "\\n";

std::string unescapeNewlines(std::string text) {
    for (auto at = text.find(kEscapedNewline); at != std::string::npos;
         at = text.find(kEscapedNewline, at + 1)) {
        text.replace(at, kEscapedNewline.size(), "\n");
    }
    return text;
}

} // namespace

int main(int argc, char *argv[]) {
    argparse::ArgumentParser parser("credits-avatar", "", argparse::default_arguments::help);
    parser.add_description("Write the credits avatar header.");
    parser.add_argument("output").help("The header to write.");
    // The defaults are applied after parsing. argparse would otherwise append them to the help.
    parser.add_argument("--magick")
        .metavar("MAGICK")
        .help("The ImageMagick command, or an empty string for no avatar. Defaults to magick.");
    parser.add_argument("--size")
        .metavar("SIZE")
        .help("Texture size in pixels. Defaults to 128.")
        .scan<'i', int>();
    parser.add_argument("--text")
        .metavar("TEXT")
        .help("The credit text. \\n breaks a line.")
        .required();
    parser.add_argument("--user")
        .metavar("USER")
        .help("The GitHub user whose avatar is used.")
        .required();
    // argparse reports usage errors only by throwing.
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << parser.usage() << "\ncredits-avatar: error: " << e.what() << '\n';
        return kUsageErrorStatus;
    }
    setupLogging(false);
    const std::filesystem::path output = parser.get<std::string>("output");
    const auto size = parser.present<int>("--size").value_or(kDefaultSize);
    const auto magick =
        parser.present<std::string>("--magick").value_or(std::string(kDefaultMagick));
    const auto text = unescapeNewlines(parser.get<std::string>("--text"));
    std::error_code error;
    if (output.has_parent_path()) {
        std::filesystem::create_directories(output.parent_path(), error);
    }
    if (error) {
        spdlog::error("Writing the credits avatar header failed. Cannot create {} ({}).",
                      output.parent_path().string(),
                      error.message());
        return EXIT_FAILURE;
    }
    const auto header = composeCreditsHeader(
        text, size, fetchAvatar(parser.get<std::string>("--user"), size, magick));
    if (const auto written = writeFile(output, header); !written) {
        spdlog::error("Writing the credits avatar header failed. {}", written.error().message);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
