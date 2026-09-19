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
 * Open a file, whether it resolves to an ark stream or a loose file.
 *
 * The routine belongs to another agent's subsystem and is declared here so ArkFile::Open() can
 * call it.
 *
 * @param pszPath The path to open, device prefix included.
 * @return The handle, or a negative value on failure.
 * @ghidraAddress 0x0055c400
 */
int OpenStreamByPath(const char *pszPath);

/**
 * Read one chunk of a file into a buffer.
 *
 * @param nFile The file to read.
 * @param nSector The chunk index.
 * @param pBuffer The destination.
 * @param nLength The number of bytes to read.
 * @ghidraAddress 0x0055c498
 */
void ReadStreamChunk(int nFile, int nSector, void *pBuffer, unsigned nLength);

/**
 * Append a component to a device path.
 *
 * A backslash is appended first when the component is not empty, and the component is normalised
 * as it is copied. The argument order is the component before the buffer, which is the image's own
 * order rather than a transcription slip.
 *
 * @param pszComponent The component to append.
 * @param pszPath The buffer to append to.
 * @ghidraAddress 0x0047dec0
 */
void AppendPathComponent(const char *pszComponent, char *pszPath);

/**
 * Close an open file, whether it is an ark stream or a loose file.
 *
 * The routine belongs to another agent's subsystem and is declared here so ArkFile::Close() can
 * call it. The whole body forwards to the SDK primitive at 0x0056af88 with the argument passed
 * through. The Ghidra program titles it ReleaseLoadFileHandle rather than this name, because the
 * bridge's naming policy rejects a title sharing every token of FileClose(). Its plate comment
 * records the pairing.
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

/**
 * Inflate a gzip member that is already in memory.
 *
 * The routine belongs to another agent's subsystem and is declared here so async.cpp can call it.
 * The source and destination windows may overlap, and every caller in the async layer relies on
 * that: the stored bytes sit against the end of the destination and the inflate runs forward over
 * the whole of it.
 *
 * The routine still carries a placeholder title in the Ghidra program. The name here is inferred
 * from its arguments and from the gzip state it writes.
 *
 * @param pSource The stored bytes.
 * @param nSourceLength The number of stored bytes.
 * @param pDest The destination.
 * @return Positive once the data is in place, and not positive on failure.
 * @ghidraAddress 0x005636a0
 */
int InflateGzBuffer(const void *pSource, int nSourceLength, void *pDest);
