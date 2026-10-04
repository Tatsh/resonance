#include "githubclient.h"

#include <format>
#include <httplib.h>
#include <regex>
#include <utility>

#include "imageerror.h"

namespace {

constexpr std::string_view kGitHubApi = "https://api.github.com";
constexpr std::string_view kWorkflowFile = "build.yml";
constexpr std::string_view kUserAgent = "resonance-build-image";
constexpr time_t kHttpTimeoutSeconds = 30;
constexpr int kMaximumRedirects = 30;
constexpr int kFirstRedirectStatus = 300;
constexpr int kFirstErrorStatus = 400;

// Reports whether a JSON object has a string member with the given value.
bool hasString(const nlohmann::json &object, const char *key, std::string_view value) {
    const auto found = object.find(key);
    return found != object.end() && found->is_string() &&
           found->get_ref<const std::string &>() == value;
}

} // namespace

GitHubClient::GitHubClient(std::string repo, std::string token)
    : repo_(std::move(repo)), token_(std::move(token)) {
}

std::expected<std::uint64_t, Error> GitHubClient::latestSuccessfulRun() const {
    const auto data =
        apiJson(std::format("{}/repos/{}/actions/workflows/{}/runs?status=completed&per_page=20",
                            kGitHubApi,
                            repo_,
                            kWorkflowFile));
    if (!data) {
        return std::unexpected(data.error());
    }
    if (data->is_object() && data->contains("workflow_runs") &&
        (*data)["workflow_runs"].is_array()) {
        for (const auto &run : (*data)["workflow_runs"]) {
            if (run.is_object() && hasString(run, "conclusion", "success") && run.contains("id") &&
                run["id"].is_number_unsigned()) {
                return run["id"].get<std::uint64_t>();
            }
        }
    }
    return artifactError(std::format("No successful runs exist for {}.", repo_));
}

std::expected<std::vector<std::uint8_t>, Error>
GitHubClient::downloadArtifact(std::uint64_t runId, const std::string &artifact) const {
    const auto data = apiJson(std::format(
        "{}/repos/{}/actions/runs/{}/artifacts?per_page=100", kGitHubApi, repo_, runId));
    if (!data) {
        return std::unexpected(data.error());
    }
    if (data->is_object() && data->contains("artifacts") && (*data)["artifacts"].is_array()) {
        for (const auto &item : (*data)["artifacts"]) {
            if (!item.is_object() || !hasString(item, "name", artifact)) {
                continue;
            }
            if (item.contains("expired") && item["expired"].is_boolean() &&
                item["expired"].get<bool>()) {
                return artifactError(std::format("Artifact {} expired.", artifact));
            }
            if (!item.contains("archive_download_url") ||
                !item["archive_download_url"].is_string()) {
                return artifactError(std::format("Artifact {} has no download URL.", artifact));
            }
            return readUrl(item["archive_download_url"].get<std::string>())
                .transform([](const std::string &body) {
                    return std::vector<std::uint8_t>(body.begin(), body.end());
                });
        }
    }
    return artifactError(std::format("Artifact {} is missing from run {}.", artifact, runId));
}

// The token is dropped once a redirect moves to another host. The presigned storage download
// requires anonymous access.
std::expected<std::string, Error> GitHubClient::readUrl(const std::string &url) const {
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
    return artifactError(std::format(
        "GitHub request failed for {}. This build-image was built without HTTPS support.", url));
#else
    static const std::regex kUrl(R"(^(https?://[^/]+)(/.*)?$)", std::regex::icase);
    auto current = url;
    std::string firstOrigin;
    auto authorize = true;
    for (int redirect = 0; redirect <= kMaximumRedirects; ++redirect) {
        std::smatch parts;
        if (!std::regex_match(current, parts, kUrl)) {
            return artifactError(std::format(
                "GitHub request failed for {}. The URL {} is not valid.", url, current));
        }
        const auto origin = parts[1].str();
        const auto path = parts[2].matched ? parts[2].str() : std::string("/");
        if (firstOrigin.empty()) {
            firstOrigin = origin;
        }
        authorize = authorize && origin == firstOrigin;
        httplib::Client client(origin);
        client.set_connection_timeout(kHttpTimeoutSeconds);
        client.set_read_timeout(kHttpTimeoutSeconds);
        httplib::Headers headers{{"Accept", "application/vnd.github+json"},
                                 {"User-Agent", std::string(kUserAgent)}};
        if (authorize) {
            headers.emplace("Authorization", "Bearer " + token_);
        }
        const auto response = client.Get(path, headers);
        if (!response) {
            return artifactError(std::format(
                "GitHub request failed for {} ({}).", url, httplib::to_string(response.error())));
        }
        if (response->status >= kFirstRedirectStatus && response->status < kFirstErrorStatus &&
            response->has_header("Location")) {
            const auto location = response->get_header_value("Location");
            current = location.starts_with('/') ? origin + location : location;
            continue;
        }
        if (response->status >= kFirstErrorStatus) {
            return artifactError(
                std::format("GitHub responded with status {} for {}.", response->status, url));
        }
        return response->body;
    }
    return artifactError(
        std::format("GitHub request failed for {} after too many redirects.", url));
#endif
}

std::expected<nlohmann::json, Error> GitHubClient::apiJson(const std::string &url) const {
    return readUrl(url).and_then(
        [&url](const std::string &body) -> std::expected<nlohmann::json, Error> {
            auto data = nlohmann::json::parse(body, nullptr, false);
            if (data.is_discarded()) {
                return artifactError(std::format("GitHub returned invalid JSON for {}.", url));
            }
            return data;
        });
}
