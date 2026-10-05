#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <expected>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <argparse/argparse.hpp>
#include <spdlog/spdlog.h>

#include "directoryvolume.h"
#include "fileio.h"
#include "githubclient.h"
#include "imagebuilder.h"
#include "imageerror.h"
#include "logging.h"
#include "sourceimage.h"
#include "ziparchive.h"

namespace Tools::BuildImage {

namespace {

namespace fs = std::filesystem;

constexpr std::string_view kProgram = "build-image";
constexpr std::string_view kDefaultRepo = "Tatsh/resonance";
constexpr std::string_view kReleaseAssetsArtifact = "release-assets-";
constexpr std::string_view kBuildTypeRelease = "Release";
constexpr std::string_view kBuildTypeDebug = "Debug";
constexpr std::string_view kExecutablePrefix = "resonance-";
constexpr std::string_view kExecutableSuffix = ".elf";
constexpr std::string_view kTargetModule = "EZMIDI.IRX";
constexpr int kUsageErrorStatus = 2;

using Payload = std::vector<std::uint8_t>;

struct Options {
    std::string buildType;
    std::optional<fs::path> ezmidiIrx;
    fs::path inputImage;
    fs::path outputCue;
    bool overwrite = false;
    bool pal = false;
    std::string repo;
    std::optional<fs::path> resonanceBin;
    std::optional<std::uint64_t> runId;
    std::optional<fs::path> systemArea;
    std::optional<std::string> token;
};

struct Payloads {
    Payload executable;
    Payload module;
};

bool isRelativeTo(const fs::path &path, const fs::path &base) {
    const auto [baseEnd, pathEnd] = std::ranges::mismatch(base, path);
    return baseEnd == base.end();
}

fs::path resolved(const fs::path &path) {
    std::error_code error;
    auto canonical = fs::weakly_canonical(path, error);
    return error ? fs::absolute(path, error) : canonical;
}

bool pathExists(const fs::path &path) {
    std::error_code error;
    return fs::exists(path, error);
}

bool pathIsDirectory(const fs::path &path) {
    std::error_code error;
    return fs::is_directory(path, error);
}

std::string baseName(std::string_view name) {
    const auto slash = name.rfind('/');
    return std::string(slash == std::string_view::npos ? name : name.substr(slash + 1));
}

std::optional<std::string> effectiveToken(const Options &options) {
    if (options.token && !options.token->empty()) {
        return options.token;
    }
    for (const auto *name : {"GITHUB_TOKEN", "GH_TOKEN"}) {
        if (const auto *value = std::getenv(name); value != nullptr && *value != '\0') {
            return std::string(value);
        }
    }
    return std::nullopt;
}

std::expected<std::string, Error> identify(const fs::path &inputImage) {
    if (pathIsDirectory(inputImage)) {
        return DirectoryVolume::identify(inputImage);
    }
    return SourceImage::open(inputImage).and_then([](const auto &image) {
        return image->bootExecutable();
    });
}

// The executable is the one member whose name matches resonance-<VERSION>-<STD><LABEL>.elf.
std::expected<Payload, Error> extractExecutable(const ZipArchive &archive,
                                                const std::string &artifact,
                                                const std::string &standardLabel) {
    const auto suffix = std::format("-{}{}", standardLabel, kExecutableSuffix);
    std::vector<std::string> matches;
    for (const auto &name : archive.names()) {
        const auto base = baseName(name);
        if (base.size() >= kExecutablePrefix.size() + suffix.size() &&
            base.starts_with(kExecutablePrefix) && base.ends_with(suffix)) {
            matches.push_back(name);
        }
    }
    if (matches.size() != 1) {
        return artifactError(std::format("Artifact {} has {} files matching {}*{}, not one.",
                                         artifact,
                                         matches.size(),
                                         kExecutablePrefix,
                                         suffix));
    }
    return archive.extract(matches.front());
}

std::expected<Payload, Error>
extractModule(const ZipArchive &archive, const std::string &artifact, const std::string &standard) {
    const auto wanted = std::format("EZMIDI-{}.IRX", standard);
    const auto names = archive.names();
    const auto found = std::ranges::find_if(
        names, [&wanted](const auto &name) { return baseName(name) == wanted; });
    if (found == names.end()) {
        return artifactError(std::format("Artifact {} lacks {}.", artifact, wanted));
    }
    return archive.extract(*found);
}

// The video standard of the downloaded builds follows the disc being rebuilt. The Release
// artifact provides the module whatever the build type.
std::expected<Payloads, Error> resolvePayloads(const Options &options) {
    if (options.resonanceBin && options.ezmidiIrx) {
        auto executable = readFile(*options.resonanceBin);
        auto module = readFile(*options.ezmidiIrx);
        if (!executable) {
            return std::unexpected(std::move(executable.error()));
        }
        if (!module) {
            return std::unexpected(std::move(module.error()));
        }
        return Payloads{std::move(*executable), std::move(*module)};
    }
    const auto token = effectiveToken(options);
    if (!token) {
        return artifactError("A token is required for downloads. Set GITHUB_TOKEN.");
    }
    const auto boots = identify(options.inputImage);
    if (!boots) {
        return std::unexpected(boots.error());
    }
    const std::string standard = *boots == Volume::kPalExecutable ? "PAL" : "NTSC";
    GitHubClient client(options.repo, *token);
    std::uint64_t runId = 0;
    if (options.runId) {
        runId = *options.runId;
    } else {
        const auto latest = client.latestSuccessfulRun();
        if (!latest) {
            return std::unexpected(latest.error());
        }
        runId = *latest;
    }
    // Each artifact is downloaded at most once.
    std::map<std::string, ZipArchive> archives;
    const auto archive =
        [&](const std::string &artifact) -> std::expected<const ZipArchive *, Error> {
        if (const auto found = archives.find(artifact); found != archives.end()) {
            return &found->second;
        }
        auto opened = client.downloadArtifact(runId, artifact).and_then(ZipArchive::open);
        if (!opened) {
            return std::unexpected(std::move(opened.error()));
        }
        return &archives.emplace(artifact, std::move(*opened)).first->second;
    };
    const auto releaseArtifact = std::format("{}{}", kReleaseAssetsArtifact, standard);
    const auto label = options.buildType == kBuildTypeRelease ?
                           std::string() :
                           std::format("-{}", options.buildType);

    auto executable =
        options.resonanceBin ?
            readFile(*options.resonanceBin) :
            archive(releaseArtifact + label).and_then([&](const ZipArchive *found) {
                return extractExecutable(*found, releaseArtifact + label, standard + label);
            });
    if (!executable) {
        return std::unexpected(std::move(executable.error()));
    }
    auto module = options.ezmidiIrx ?
                      readFile(*options.ezmidiIrx) :
                      archive(releaseArtifact).and_then([&](const ZipArchive *found) {
                          return extractModule(*found, releaseArtifact, standard);
                      });
    if (!module) {
        return std::unexpected(std::move(module.error()));
    }
    return Payloads{std::move(*executable), std::move(*module)};
}

std::expected<std::unique_ptr<Volume>, Error> openVolume(const Options &options) {
    if (!pathIsDirectory(options.inputImage)) {
        return SourceImage::open(options.inputImage).transform([](auto image) {
            return std::unique_ptr<Volume>(std::move(image));
        });
    }
    std::optional<Payload> systemArea;
    if (options.systemArea) {
        auto data = readFile(*options.systemArea);
        if (!data) {
            return std::unexpected(std::move(data.error()));
        }
        systemArea = std::move(*data);
    }
    return DirectoryVolume::create(options.inputImage, std::move(systemArea))
        .transform([](auto volume) { return std::unique_ptr<Volume>(std::move(volume)); });
}

std::expected<std::uint32_t, Error> run(const Options &options) {
    if (Volume::lowerAscii(options.outputCue.extension().string()) != ".cue") {
        return discImageError(
            std::format("The output {} is not a cue sheet.", options.outputCue.string()));
    }
    auto image = options.outputCue;
    image.replace_extension(".bin");
    if (!options.overwrite && (pathExists(options.outputCue) || pathExists(image))) {
        return discImageError(std::format("The output {} exists.", options.outputCue.string()));
    }
    const auto input = resolved(options.inputImage);
    if (resolved(image) == input || resolved(options.outputCue) == input) {
        return discImageError("Output must differ from the input.");
    }
    if (pathIsDirectory(options.inputImage) && isRelativeTo(resolved(image), input)) {
        return discImageError("Output must lie outside the disc root.");
    }
    auto payloads = resolvePayloads(options);
    if (!payloads) {
        return std::unexpected(std::move(payloads.error()));
    }
    const auto source = openVolume(options);
    if (!source) {
        return std::unexpected(source.error());
    }
    const auto target = (*source)->bootExecutable();
    if (!target) {
        return std::unexpected(target.error());
    }
    const auto isPal = *target == Volume::kPalExecutable;
    if (options.pal != isPal) {
        return discImageError(
            std::format("The original boots {}. A PAL build requires the European disc, and an "
                        "NTSC build requires the North American disc.",
                        *target));
    }
    spdlog::info("Original is the {} release ({}).", isPal ? "PAL" : "NTSC-U/C", *target);
    std::vector<Replacement> replacements;
    replacements.push_back({*target, std::move(payloads->executable)});
    replacements.push_back({std::string(kTargetModule), std::move(payloads->module)});
    return ImageBuilder::plan(**source, std::move(replacements))
        .and_then([&options](ImageBuilder builder) { return builder.write(options.outputCue); });
}

int usageError(const argparse::ArgumentParser &parser, const std::string &message) {
    std::cerr << parser.usage() << '\n' << kProgram << ": error: " << message << '\n';
    return kUsageErrorStatus;
}

} // namespace

} // namespace Tools::BuildImage

int main(int argc, char *argv[]) {
    using namespace Tools;
    using namespace Tools::BuildImage;
    argparse::ArgumentParser parser(std::string(kProgram), "", argparse::default_arguments::help);
    parser.add_description(
        "Rebuild a FreQuency CD image with replacement binaries. The raw MODE2/2352 bin is "
        "written beside the cue sheet, with the same name and a .bin suffix.");
    parser.add_argument("input_image")
        .help("Original image in cue, bin, or ISO form, or the disc root directory (the directory "
              "with SYSTEM.CNF).");
    parser.add_argument("output_cue")
        .help("Destination cue sheet path. Required unless --identify is given.")
        .nargs(argparse::nargs_pattern::optional);
    parser.add_argument("--identify")
        .help("Print the name of the executable the original boots, and exit.")
        .flag();
    parser.add_argument("--pal")
        .help("The executable is a PAL build. The European disc (SCES_507.91) requires the flag, "
              "and the North American disc refuses it.")
        .flag();
    parser.add_argument("--build-type")
        .metavar("BUILD_TYPE")
        .help("Build type of the downloaded executable. The module always comes from the Release "
              "build.")
        .default_value(std::string(kBuildTypeRelease))
        .nargs(1)
        .choices(std::string(kBuildTypeRelease), std::string(kBuildTypeDebug));
    parser.add_argument("--debug").help("Enable debug logging.").flag();
    parser.add_argument("--ezmidi-irx")
        .metavar("EZMIDI_IRX")
        .help("Local module file, bypassing the artifact download.");
    parser.add_argument("--overwrite").help("Replace the output files when present.").flag();
    parser.add_argument("--repo")
        .metavar("REPO")
        .help("Repository with the build artifacts.")
        .default_value(std::string(kDefaultRepo))
        .nargs(1);
    parser.add_argument("--resonance-bin")
        .metavar("RESONANCE_BIN")
        .help("Local executable file, bypassing the artifact download.");
    parser.add_argument("--run-id")
        .metavar("RUN_ID")
        .help("Actions run to fetch. The latest successful run is used when omitted.");
    parser.add_argument("--system-area")
        .metavar("SYSTEM_AREA")
        .help("The first 12 sectors of the original disc (24576 bytes, the boot logo), for a disc "
              "root input. The sectors are zero otherwise.");
    parser.add_argument("--token").metavar("TOKEN").help(
        "GitHub token. GITHUB_TOKEN or GH_TOKEN provides the value when the flag is absent.");
    // argparse reports usage errors only by throwing.
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &e) {
        return usageError(parser, e.what());
    }

    Options options;
    options.inputImage = parser.get<std::string>("input_image");
    if (!pathExists(options.inputImage)) {
        return usageError(
            parser, std::format("The input image {} does not exist.", options.inputImage.string()));
    }
    for (auto [name, field] : {std::pair{"--ezmidi-irx", &options.ezmidiIrx},
                               std::pair{"--resonance-bin", &options.resonanceBin},
                               std::pair{"--system-area", &options.systemArea}}) {
        if (const auto value = parser.present<std::string>(name)) {
            std::error_code error;
            if (!fs::is_regular_file(*value, error)) {
                return usageError(parser,
                                  std::format("The {} file {} does not exist.", name, *value));
            }
            *field = *value;
        }
    }
    if (const auto runId = parser.present<std::string>("--run-id")) {
        std::uint64_t value = 0;
        const auto *end = runId->data() + runId->size();
        if (const auto [last, error] = std::from_chars(runId->data(), end, value);
            error != std::errc() || last != end) {
            return usageError(parser,
                              std::format("The --run-id value '{}' is not an integer.", *runId));
        }
        options.runId = value;
    }
    options.buildType = parser.get<std::string>("--build-type");
    options.overwrite = parser.get<bool>("--overwrite");
    options.pal = parser.get<bool>("--pal");
    options.repo = parser.get<std::string>("--repo");
    options.token = parser.present<std::string>("--token");
    setupLogging(parser.get<bool>("--debug"));

    if (parser.get<bool>("--identify")) {
        const auto boots = identify(options.inputImage);
        if (!boots) {
            spdlog::error("Cannot identify the original. {}", boots.error().message);
            return EXIT_FAILURE;
        }
        std::cout << *boots << '\n';
        return EXIT_SUCCESS;
    }
    const auto output = parser.present<std::string>("output_cue");
    if (!output) {
        return usageError(parser, "The output cue sheet is required unless --identify is given.");
    }
    options.outputCue = *output;
    auto image = options.outputCue;
    image.replace_extension(".bin");
    const auto cueExisted = pathExists(options.outputCue);
    const auto imageExisted = pathExists(image);
    const auto sectors = run(options);
    if (!sectors) {
        std::error_code ignored;
        if (!cueExisted) {
            fs::remove(options.outputCue, ignored);
        }
        if (!imageExisted) {
            fs::remove(image, ignored);
        }
        spdlog::error("Image build failed. {}", sectors.error().message);
        return EXIT_FAILURE;
    }
    std::cout << std::format("Wrote {} ({} sectors).\n", options.outputCue.string(), *sectors);
    return EXIT_SUCCESS;
}
