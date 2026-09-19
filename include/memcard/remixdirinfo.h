#pragma once

#include "os/hxstr.h"

/** Entries one remix save directory accepts before SaveRemixMCT considers it full. */
constexpr int kMaxRemixesPerDirectory = 13;

/**
 * Characters of a remix save directory name that precede its number.
 *
 * `/BASCUS-97125r` at `0x007db0d8` is exactly this long, and the parse of the directory number
 * starts at this offset into the name.
 */
constexpr int kRemixDirNumberOffset = 14;

/**
 * Summary of one remix save directory, parsed out of the index file inside it.
 *
 * SaveRemixMCT builds a `std::vector` of these records at `+0x3c`, one per directory the listing
 * of `/BASCUS-97125r*` found, and then chooses the directory the save lands in from the vector.
 * The record is not polymorphic, so it emits no RTTI descriptor, no `__FILE__` path survives for
 * its translation unit, and no literal in the image identifies it. Neither the class name nor any
 * of the four field names is attested, so all five follow the required style. Every one of the
 * four fields is recovered from the disassembly.
 *
 * The record is a plain data aggregate with no behaviour, so it is a `struct` with public members.
 * Its copy constructor is the implicit one, which SaveRemixMCT::AppendDirInfo relies on when it
 * copies a stack temporary into the vector.
 *
 * Three routines fix the layout between them. SaveRemixMCT::AppendDirInfo at `0x00179c28` writes
 * name, dirNumber, and entryCount and zeroes highestFileNumber. SaveRemixMCT::OnFileLoaded at
 * `0x0017ad70` compares name against the directory being read, raises highestFileNumber to the
 * largest file number the directory's index lists, and tests entryCount against
 * kMaxRemixesPerDirectory. SaveRemixMCT::ChooseTargetDir at `0x00179d60` reads entryCount and
 * dirNumber.
 */
struct RemixDirInfo {
    HxStr name;            /*!< Directory name, as the card listing reported it. +0x00 */
    int dirNumber;         /*!< Number parsed out of name at kRemixDirNumberOffset. +0x08 */
    int entryCount;        /*!< Elements the directory's index file lists. +0x0c */
    int highestFileNumber; /*!< Largest decimal file name among those elements. +0x10 */
};
