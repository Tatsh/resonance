#include "nameirx.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <elfio/elfio.hpp>
#include <format>
#include <spanstream>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

namespace {

constexpr std::array<std::uint8_t, 4> kElfMagic{0x7f, 'E', 'L', 'F'};
constexpr std::size_t kElfSectionHeaderOffset = 32;
constexpr std::size_t kSectionHeaderSize = 40;
constexpr std::size_t kProgramHeaderSize = 32;
constexpr ELFIO::Elf_Word kSectionTypeIopModule = 0x70000080;
constexpr ELFIO::Elf_Word kProgramTypeIopModule = 0x70000080;
// The fixed part of the `.iopmod` record comprises the ModuleInfo address, the entry, the gp value,
// the text, data, and bss sizes, and the version.
constexpr std::size_t kIopModuleFixedSize = 6 * sizeof(std::uint32_t) + sizeof(std::uint16_t);
constexpr std::size_t kNameLimit = 256;

// A section header has ten words and a program header has eight, listed in file order.
using SectionHeader = std::array<std::uint32_t, 10>;
using ProgramHeader = std::array<std::uint32_t, 8>;
enum SectionField : std::size_t { kSectionOffset = 4, kSectionSize = 5 };
enum ProgramField : std::size_t { kProgramType = 0, kProgramOffset = 1, kProgramFileSize = 4 };

std::unexpected<Error> invalid(std::string message) {
    return std::unexpected(Error{ErrorCode::InvalidInput, std::move(message)});
}

bool inRange(std::span<const std::uint8_t> data, std::size_t offset, std::size_t size) {
    return offset <= data.size() && size <= data.size() - offset;
}

std::uint32_t readWord(std::span<const std::uint8_t> data, std::size_t offset) {
    std::uint32_t value = 0;
    for (std::size_t i = sizeof(std::uint32_t); i-- > 0;) {
        value = (value << 8) | data[offset + i];
    }
    return value;
}

template <std::size_t N>
std::expected<void, Error> writeWords(std::span<std::uint8_t> data,
                                      std::size_t offset,
                                      const std::array<std::uint32_t, N> &words) {
    if (!inRange(data, offset, N * sizeof(std::uint32_t))) {
        return invalid(std::format("Offset {:#x} is past the end of the file.", offset));
    }
    for (const auto word : words) {
        for (std::size_t i = 0; i < sizeof(std::uint32_t); ++i) {
            data[offset++] = static_cast<std::uint8_t>(word >> (8 * i));
        }
    }
    return {};
}

// Converts a module address inside a section with file contents to a file offset.
std::expected<std::size_t, Error> fileOffset(const ELFIO::elfio &elf, std::uint32_t address) {
    for (const auto &section : elf.sections) {
        if (section->get_type() == ELFIO::SHT_PROGBITS && section->get_address() <= address &&
            address - section->get_address() < section->get_size()) {
            return section->get_offset() + address - section->get_address();
        }
    }
    return invalid(std::format("No section covers module address {:#x}.", address));
}

// Returns the bytes up to the first NUL, or every byte when there is no NUL.
std::span<const std::uint8_t> untilNul(std::span<const std::uint8_t> bytes) {
    return bytes.first(static_cast<std::size_t>(std::ranges::find(bytes, 0) - bytes.begin()));
}

SectionHeader sectionHeader(const ELFIO::section &section) {
    return {section.get_name_string_offset(),
            section.get_type(),
            static_cast<std::uint32_t>(section.get_flags()),
            static_cast<std::uint32_t>(section.get_address()),
            static_cast<std::uint32_t>(section.get_offset()),
            static_cast<std::uint32_t>(section.get_size()),
            section.get_link(),
            section.get_info(),
            static_cast<std::uint32_t>(section.get_addr_align()),
            static_cast<std::uint32_t>(section.get_entry_size())};
}

ProgramHeader programHeader(const ELFIO::segment &segment) {
    return {segment.get_type(),
            static_cast<std::uint32_t>(segment.get_offset()),
            static_cast<std::uint32_t>(segment.get_virtual_address()),
            static_cast<std::uint32_t>(segment.get_physical_address()),
            static_cast<std::uint32_t>(segment.get_file_size()),
            static_cast<std::uint32_t>(segment.get_memory_size()),
            segment.get_flags(),
            static_cast<std::uint32_t>(segment.get_align())};
}

} // namespace

std::expected<std::vector<std::uint8_t>, Error> nameIrx(std::span<const std::uint8_t> data) {
    if (data.size() < kElfMagic.size() ||
        !std::ranges::equal(data.first(kElfMagic.size()), kElfMagic)) {
        return invalid("Not an ELF file.");
    }
    // ELFIO reads from a stream of char.
    std::ispanstream stream(std::span(reinterpret_cast<const char *>(data.data()), data.size()));
    ELFIO::elfio elf;
    if (!elf.load(stream) || elf.get_class() != ELFIO::ELFCLASS32 ||
        elf.get_encoding() != ELFIO::ELFDATA2LSB) {
        return invalid("Not a little-endian 32-bit ELF file.");
    }
    const auto iopModule = std::ranges::find_if(elf.sections, [](const auto &section) {
        return section->get_type() == kSectionTypeIopModule;
    });
    if (iopModule == elf.sections.end()) {
        return invalid("No .iopmod section.");
    }
    const auto iopModuleIndex = (*iopModule)->get_index();
    const std::size_t recordOffset = (*iopModule)->get_offset();
    const std::size_t recordSize = (*iopModule)->get_size();
    if (!inRange(data, recordOffset, recordSize)) {
        return invalid("The .iopmod record is past the end of the file.");
    }
    const auto record = data.subspan(recordOffset, recordSize);
    if (record.size() > kIopModuleFixedSize &&
        !untilNul(record.subspan(kIopModuleFixedSize)).empty()) {
        return std::vector<std::uint8_t>(data.begin(), data.end());
    }
    if (record.size() < sizeof(std::uint32_t)) {
        return invalid("The .iopmod record is truncated.");
    }
    const auto moduleInfo = fileOffset(elf, readWord(record, 0));
    if (!moduleInfo) {
        return std::unexpected(moduleInfo.error());
    }
    if (!inRange(data, *moduleInfo, sizeof(std::uint32_t))) {
        return invalid("The ModuleInfo record is past the end of the file.");
    }
    const auto nameOffset = fileOffset(elf, readWord(data, *moduleInfo));
    if (!nameOffset) {
        return std::unexpected(nameOffset.error());
    }
    const auto nameStart = std::min(*nameOffset, data.size());
    const auto name =
        untilNul(data.subspan(nameStart, std::min(kNameLimit, data.size() - nameStart)));

    const auto load = std::ranges::find_if(
        elf.segments, [](const auto &segment) { return segment->get_type() == ELFIO::PT_LOAD; });
    if (load == elf.segments.end()) {
        return invalid("No loadable segment.");
    }
    const std::size_t alignment = std::max<ELFIO::Elf_Xword>((*load)->get_align(), 1);
    const std::size_t oldRest = (*load)->get_offset();
    if (oldRest < recordOffset + recordSize) {
        return invalid("The loadable segment overlaps the .iopmod record.");
    }
    if (oldRest > data.size()) {
        return invalid("The loadable segment is past the end of the file.");
    }
    const auto fixedSize = std::min(record.size(), kIopModuleFixedSize);
    const auto newRecordSize = static_cast<std::uint32_t>(fixedSize + name.size() + 1);
    const auto newRest = (recordOffset + newRecordSize + alignment - 1) / alignment * alignment;
    const auto delta = static_cast<std::uint32_t>(newRest - oldRest);

    std::vector<std::uint8_t> out(data.begin(), data.begin() + recordOffset);
    out.insert(out.end(), record.begin(), record.begin() + fixedSize);
    out.insert(out.end(), name.begin(), name.end());
    out.resize(newRest);
    out.insert(out.end(), data.begin() + oldRest, data.end());

    const auto sectionHeaderOffset = static_cast<std::uint32_t>(elf.get_sections_offset());
    auto newSectionHeaderOffset = sectionHeaderOffset;
    if (sectionHeaderOffset >= oldRest) {
        newSectionHeaderOffset += delta;
    }
    if (auto written = writeWords(out, kElfSectionHeaderOffset, std::array{newSectionHeaderOffset});
        !written) {
        return std::unexpected(written.error());
    }
    for (const auto &segment : elf.segments) {
        auto program = programHeader(*segment);
        if (program[kProgramType] == kProgramTypeIopModule) {
            program[kProgramFileSize] = newRecordSize;
        } else if (program[kProgramOffset] >= oldRest) {
            program[kProgramOffset] += delta;
        }
        const auto offset = elf.get_segments_offset() + segment->get_index() * kProgramHeaderSize;
        if (auto written = writeWords(out, offset, program); !written) {
            return std::unexpected(written.error());
        }
    }
    for (const auto &section : elf.sections) {
        auto header = sectionHeader(*section);
        if (section->get_index() == iopModuleIndex) {
            header[kSectionSize] = newRecordSize;
        } else if (header[kSectionOffset] >= oldRest) {
            header[kSectionOffset] += delta;
        }
        const auto offset = newSectionHeaderOffset + section->get_index() * kSectionHeaderSize;
        if (auto written = writeWords(out, offset, header); !written) {
            return std::unexpected(written.error());
        }
    }
    spdlog::debug("Named the module `{}`.", std::string(name.begin(), name.end()));
    return out;
}
