#pragma once

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "os/hxstr.h"

/** Entries one remix save directory accepts before SaveRemixMCT considers it full. */
constexpr int kMaxRemixesPerDirectory = 13;

/** Bytes of the stack buffer a payload file number is formatted into. */
constexpr int kRemixFileNumberTextSize = 16;

/** Payload file name of the first remix in a fresh directory. */
constexpr char kFirstRemixPayloadFileName[] = "1";

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
 * The record is not polymorphic and emits no RTTI descriptor. No `__FILE__` path is present for
 * its translation unit, and no literal in the image identifies it. Neither the class name nor any
 * of the four field names is attested. Every one of the four fields is recovered from the
 * disassembly.
 *
 * The record is a `struct` with public members. SaveRemixMCT::AppendDirInfo copies a stack
 * temporary into the vector through the implicit copy constructor.
 *
 * Three routines fix the layout between them. SaveRemixMCT::AppendDirInfo at `0x00179c28` writes
 * name, dirNumber, and entryCount and zeroes highestFileNumber. SaveRemixMCT::OnFileLoaded at
 * `0x0017ad70` compares name against the directory being read, raises highestFileNumber to the
 * largest file number the directory's index lists, and tests entryCount against
 * kMaxRemixesPerDirectory. SaveRemixMCT::ChooseTargetDir at `0x00179d60` reads entryCount and
 * dirNumber. Every comparison of the three counts is an unsigned one.
 *
 * The constructor and the two static members are inline. Their out-of-line copies at `0x00186828`,
 * `0x001868b8`, and `0x00186978` have no caller.
 */
struct RemixDirInfo {
    /**
     * Record a directory the listing found, with no file number seen yet.
     *
     * The body assigns the number, because the image parses it after storing the entry count.
     *
     * @param dirName The directory name.
     * @param nEntryCount Elements the directory's index file lists.
     * @ghidraAddress 0x00186828
     */
    RemixDirInfo(const HxStr &dirName, unsigned int nEntryCount)
        : name(dirName), entryCount(nEntryCount), highestFileNumber(0) {
        dirNumber =
            atoi((name.mStr != nullptr ? name.mStr : g_szEmptyString) + kRemixDirNumberOffset);
    }

    /**
     * Raise the highest file number of the first record named after a directory.
     *
     * SaveRemixMCT::OnFileLoaded() expands it once per index element.
     *
     * @param infos The records.
     * @param dirName The directory the file number came from.
     * @param nFileNumber The file number.
     * @ghidraAddress 0x001868b8
     */
    static void RaiseHighestFileNumber(std::vector<RemixDirInfo> &infos,
                                       const HxStr &dirName,
                                       unsigned int nFileNumber) {
        for (std::vector<RemixDirInfo>::iterator it = infos.begin(); it != infos.end(); ++it) {
            if (it->name == dirName) {
                it->highestFileNumber = std::max(nFileNumber, it->highestFileNumber);
                return;
            }
        }
    }

    /**
     * Name the payload file a new remix takes.
     *
     * One past the highest file number of the first directory with room, or
     * kFirstRemixPayloadFileName when no directory has room. SaveRemixMCT::OnListDir() and
     * OnFileLoaded() expand it.
     *
     * @param infos The records.
     * @return The file name.
     * @ghidraAddress 0x00186978
     */
    static HxStr NextPayloadFileName(const std::vector<RemixDirInfo> &infos) {
        for (std::vector<RemixDirInfo>::const_iterator it = infos.begin(); it != infos.end();
             ++it) {
            if (it->entryCount < kMaxRemixesPerDirectory) {
                char szNumber[kRemixFileNumberTextSize];
                sprintf(szNumber, "%d", it->highestFileNumber + 1);
                return HxStr(szNumber);
            }
        }
        return HxStr(kFirstRemixPayloadFileName);
    }

    HxStr name;                     /*!< Directory name, as the card listing reported it. +0x00 */
    unsigned int dirNumber;         /*!< Parsed out of name at kRemixDirNumberOffset. +0x08 */
    unsigned int entryCount;        /*!< Elements the directory's index file lists. +0x0c */
    unsigned int highestFileNumber; /*!< Largest decimal file name among the elements. +0x10 */
};
