#pragma once

/** Extension that routes a load through the decompressing path. */
constexpr char kGzExtension[] = ".gz";

/**
 * Read a whole file.
 *
 * A path whose last three characters match kGzExtension, tested without regard to case, is handed
 * to LoadGzFile() instead. Any other path is opened, measured, and read in one call.
 *
 * With no buffer supplied the routine allocates one, from the selected zone when there is one and
 * from MemAllocTagged() otherwise. With a buffer supplied that is too small the file is closed and
 * the load reports null.
 *
 * @param pszPath The file to read.
 * @param pBuffer The destination, or null to have one allocated.
 * @param nBufferSize The destination size, which is ignored when pBuffer is null.
 * @param pnSize Receives the file size, and is written even when the allocation failed.
 * @return The destination, or null when the file could not be opened or read.
 * @ghidraAddress 0x00555538
 */
void *LoadWholeFile(const char *pszPath, void *pBuffer, unsigned nBufferSize, unsigned *pnSize);

/**
 * Read a whole compressed file, decompressing it.
 *
 * The size queried and reported is the decompressed size, and the reader inflates the whole file
 * in one call. Whether the query and the read address an ark stream or a loose file depends on
 * UsingArkFiles().
 *
 * A supplied buffer that is too small is fatal here rather than a null report, which is the one
 * behavioural difference from LoadWholeFile(). The file is not closed on the success path.
 *
 * @param pszPath The file to read.
 * @param pBuffer The destination, or null to have one allocated.
 * @param nBufferSize The destination size, which is ignored when pBuffer is null.
 * @param pnSize Receives the decompressed size.
 * @return The destination, or null when the file could not be opened.
 * @ghidraAddress 0x00555678
 */
void *LoadGzFile(const char *pszPath, void *pBuffer, unsigned nBufferSize, unsigned *pnSize);

/**
 * Whether data is being read from the disc.
 *
 * The flag is one word of a block of eight boot options at 0x0070bf10 that each have an accessor
 * of this shape. The retail configurator at 0x0050f030, which InitIop() calls first, writes the
 * block in one pass and sets this word to 1 in the same instruction run that sets GetHostMode() to
 * kHostModeCdOnly and UsingArkFiles() to 1. The `.data` default is zero, which is the
 * host-development configuration. It is a separate word from the one UsingArkFiles() reads at
 * 0x0070bf14.
 *
 * Both uses agree with that reading. InitAsync() starts the worker thread only when this reports
 * the disc, because a host-link read needs no latency hiding, and ArkFile::Open() searches the
 * path for a device prefix only then, because a prefix such as `cdrom0:` exists on no host path.
 *
 * The accessor belongs to another agent's subsystem.
 *
 * @return Non-zero when data is read from the disc.
 * @ghidraAddress 0x0050efd0
 */
int UsingCdMedia();

/**
 * Close an open file, whether it is an ark stream or a loose file.
 *
 * The routine belongs to another agent's subsystem and is declared here so ArkFile::Close() can
 * call it.
 *
 * @param nFile The file to close.
 * @ghidraAddress 0x0055c438
 */
void CloseLoadFile(int nFile);

/**
 * Report the decompressed size of the file an ark stream reads.
 *
 * The routine clears bit 0x4000 from the handle, finds the matching record in the stream table,
 * and reports its directory entry's decompressed size. Nothing about it is specific to
 * compression. LoadGzFile() simply needs that size, and the ark reader at 0x0055a280 uses the same
 * entry's stored size to measure how much of the stream is left.
 *
 * @param nStream The ark stream handle.
 * @return The decompressed size in bytes, or -1 when no record has that handle.
 * @ghidraAddress 0x0055bf88
 */
int GetArkStreamInflatedSize(int nStream);

/**
 * Report the decompressed size of a compressed loose file.
 *
 * @param nFile The file.
 * @return The decompressed size in bytes.
 * @ghidraAddress 0x005638c8
 */
unsigned GetGzFileSize(int nFile);

/**
 * Decompress a whole file into a buffer.
 *
 * @param nFile The file to read.
 * @param pBuffer The destination, which must take the whole decompressed size.
 * @ghidraAddress 0x005635b8
 */
void InflateGzFileWhole(int nFile, void *pBuffer);
