#pragma once

#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

#include "error.h"

namespace Tools::BuildImage {

/** Reader of the stored and deflated members of a zip archive in memory. */
class ZipArchive {
public:
    /**
     * Read the central directory of an archive.
     *
     * @param data Archive bytes. The archive takes them over.
     * @return The archive, or an error when the bytes are not a zip archive.
     */
    static std::expected<ZipArchive, Error> open(std::vector<std::uint8_t> data);

    /**
     * List the member names in central directory order.
     *
     * @return Member names, with their directory parts.
     */
    [[nodiscard]] std::vector<std::string> names() const;

    /**
     * Extract one member.
     *
     * @param name Member name, as names() lists it.
     * @return Member contents, or an error when the member is missing or damaged.
     */
    [[nodiscard]] std::expected<std::vector<std::uint8_t>, Error>
    extract(const std::string &name) const;

private:
    struct Member {
        std::string name;
        std::uint16_t method = 0;
        std::uint32_t crc = 0;
        std::uint32_t compressedSize = 0;
        std::uint32_t size = 0;
        std::uint32_t localOffset = 0;
    };

    explicit ZipArchive(std::vector<std::uint8_t> data);

    std::vector<std::uint8_t> data_;
    std::vector<Member> members_;
};

} // namespace Tools::BuildImage
