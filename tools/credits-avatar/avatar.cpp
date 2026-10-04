#include "avatar.h"

#include <array>
#include <cstdio>
#include <fcntl.h>
#include <format>
#include <httplib.h>
#include <memory>
#include <spawn.h>
#include <spdlog/spdlog.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace {

struct FileCloser {
    void operator()(std::FILE *file) const {
        std::fclose(file);
    }
};

constexpr std::string_view kAvatarHost = "https://github.com";
constexpr int kHttpTimeoutSeconds = 15;
constexpr int kHttpOk = 200;
constexpr std::size_t kBytesPerTexel = 4;
constexpr std::size_t kAlphaIndex = 3;
constexpr std::size_t kReadChunkSize = 65536;

std::optional<std::string> download(const std::string &path, const std::string &url) {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::Client client{std::string(kAvatarHost)};
    client.set_connection_timeout(kHttpTimeoutSeconds);
    client.set_read_timeout(kHttpTimeoutSeconds);
    client.set_follow_location(true);
    if (auto response = client.Get(path); response && response->status == kHttpOk) {
        return std::move(response->body);
    }
    spdlog::warn("Cannot fetch {}. The credit has no avatar.", url);
#else
    spdlog::warn("HTTPS is unavailable without OpenSSL. Cannot fetch {}. The credit has no "
                 "avatar.",
                 url);
#endif
    return std::nullopt;
}

// Runs ImageMagick with the image on standard input and returns its standard output, like
// subprocess.run with capture_output and check. Standard error is discarded.
std::optional<std::vector<std::uint8_t>>
convert(const std::string &magick, const std::string &image, int size) {
    const std::unique_ptr<std::FILE, FileCloser> input(std::tmpfile());
    if (!input || std::fwrite(image.data(), 1, image.size(), input.get()) != image.size() ||
        std::fflush(input.get()) != 0 || std::fseek(input.get(), 0, SEEK_SET) != 0) {
        return std::nullopt;
    }
    std::array<int, 2> output{};
    if (pipe(output.data()) != 0) {
        return std::nullopt;
    }
    const auto geometry = std::format("{}x{}!", size, size);
    std::array<std::string, 7> arguments{magick, "-", "-resize", geometry, "-depth", "8", "rgba:-"};
    std::array<char *, arguments.size() + 1> argv{};
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        argv[i] = arguments[i].data();
    }
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, fileno(input.get()), STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&actions, output[1], STDOUT_FILENO);
    posix_spawn_file_actions_addopen(&actions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    posix_spawn_file_actions_addclose(&actions, output[0]);
    pid_t child = 0;
    const auto spawned =
        posix_spawnp(&child, magick.c_str(), &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(output[1]);
    std::vector<std::uint8_t> texels;
    if (spawned == 0) {
        std::array<std::uint8_t, kReadChunkSize> chunk{};
        ssize_t count = 0;
        while ((count = read(output[0], chunk.data(), chunk.size())) > 0) {
            texels.insert(texels.end(), chunk.begin(), chunk.begin() + count);
        }
    }
    close(output[0]);
    if (spawned != 0) {
        return std::nullopt;
    }
    int status = 0;
    if (waitpid(child, &status, 0) != child || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        return std::nullopt;
    }
    return texels;
}

} // namespace

std::optional<std::vector<std::uint8_t>>
fetchAvatar(const std::string &user, int size, const std::string &magick) {
    if (magick.empty()) {
        spdlog::warn("ImageMagick is not installed. The credit has no avatar.");
        return std::nullopt;
    }
    const auto path = std::format("/{}.png?size={}", user, size);
    const auto url = std::string(kAvatarHost) + path;
    const auto image = download(path, url);
    if (!image) {
        return std::nullopt;
    }
    auto texels = convert(magick, *image, size);
    if (!texels) {
        spdlog::warn("Cannot convert the avatar from {}. The credit has no avatar.", url);
        return std::nullopt;
    }
    if (texels->size() != static_cast<std::size_t>(size) * size * kBytesPerTexel) {
        spdlog::warn("The converted avatar has the wrong size. The credit has no avatar.");
        return std::nullopt;
    }
    for (auto i = kAlphaIndex; i < texels->size(); i += kBytesPerTexel) {
        (*texels)[i] = static_cast<std::uint8_t>(((*texels)[i] + 1) >> 1);
    }
    return texels;
}
