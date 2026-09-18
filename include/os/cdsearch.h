#pragma once

/**
 * Result of a disc directory search.
 *
 * This is the PlayStation 2 SDK's `sceCdlFILE`, declared here only so ArkFile::Open() can call the
 * search below. Only the sector is read, so the rest of the record is reserved rather than
 * reconstructed.
 */
struct CdFile {
    int mLsn;                    /*!< The file's first sector on the disc. +0x00 */
    unsigned char mReserved[32]; /*!< The size, name, and date the search also fills in. +0x04 */
};

/**
 * Find a file in the disc directory.
 *
 * The routine belongs to the PlayStation 2 SDK. ArkFile::Open() passes the archive path with its
 * device prefix removed, and reports failure through `sceCdSearchFile failed on: %s`.
 *
 * @param pFile Receives the result.
 * @param pszName The path to find, without a device prefix.
 * @return Non-zero on success.
 * @ghidraAddress 0x004ff620
 */
int sceCdSearchFile(CdFile *pFile, const char *pszName);
