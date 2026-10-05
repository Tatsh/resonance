#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "error.h"

namespace Tools::BuildImage {

/** Client of the GitHub Actions API for the build artifacts of one repository. */
class GitHubClient {
public:
    /**
     * Create a client.
     *
     * @param repo Repository in owner and name form.
     * @param token Bearer token for the requests.
     */
    GitHubClient(std::string repo, std::string token);

    /**
     * Identify the latest successful run of the build workflow.
     *
     * @return Run identifier, or an error when the request fails or no successful run exists.
     */
    [[nodiscard]] std::expected<std::uint64_t, Error> latestSuccessfulRun() const;

    /**
     * Fetch one artifact archive from a run.
     *
     * @param runId Actions run with the artifact.
     * @param artifact Artifact name.
     * @return Archive bytes, or an error when the request fails or the artifact is missing or
     * expired.
     */
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, Error>
    downloadArtifact(std::uint64_t runId, const std::string &artifact) const;

private:
    [[nodiscard]] std::expected<std::string, Error> readUrl(const std::string &url) const;
    [[nodiscard]] std::expected<nlohmann::json, Error> apiJson(const std::string &url) const;

    std::string repo_;
    std::string token_;
};

} // namespace Tools::BuildImage
