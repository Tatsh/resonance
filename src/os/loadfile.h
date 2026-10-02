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
 * The CD drive is synchronised first, and the path is opened read-only through sceOpen().
 *
 * @param pszPath The path to open, device prefix included.
 * @return The handle, or a negative value on failure.
 * @ghidraAddress 0x0055c400
 */
int OpenStreamByPath(const char *pszPath);

/**
 * Read one chunk of a file into a buffer.
 *
 * The chunk index is mapped through ArkfileLogicalToPhysicalSector() before the seek, even though
 * ReadArkStreamThroughCache(), the one caller, has already mapped it. An optimized archive is
 * therefore read at a doubly mapped position.
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
 * The whole body forwards to the SDK primitive at 0x0056af88 with the argument passed through.
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
 * Vendored gzip glue, declared by title only. The body stores the file in the bundled gzip code's
 * input-descriptor global at `0x00761488`, copies `unknown` into its input-name global at
 * `0x00761490`, and drives the gzip routines at `0x006125b8`, `0x00612468`, `0x00562f88`,
 * `0x0061d778`, and `0x006125d0`. Both exits close the file with FileClose() unless the descriptor
 * reads -1. The failure exit leaves -1 in the result register, which LoadGzFile(), the one caller,
 * ignores.
 *
 * @param nFile The file to read.
 * @param pBuffer The destination, which must take the whole decompressed size.
 * @ghidraAddress 0x005635b8
 */
extern "C" void InflateGzFileWhole(int nFile, void *pBuffer);

/**
 * Inflate a gzip member that is already in memory.
 *
 * The routine belongs to another agent's subsystem and is declared here so async.cpp can call it.
 * The source and destination windows may overlap, and every caller in the async layer relies on
 * that: the stored bytes sit against the end of the destination and the inflate runs forward over
 * the whole of it.
 *
 * The name is inferred from its arguments and from the gzip state it writes.
 *
 * @param pSource The stored bytes.
 * @param nSourceLength The number of stored bytes.
 * @param pDest The destination.
 * @return Positive once the data is in place, and not positive on failure.
 * @ghidraAddress 0x005636a0
 */
int InflateGzBuffer(const void *pSource, int nSourceLength, void *pDest);

/**
 * Report the stored length of a file.
 *
 * The path is opened, measured by seeking to its end, rewound, and closed. A gzip file reports its
 * compressed size, unlike GetUncompressedFileLength(). The name is inferred.
 *
 * @param pszPath The file to measure.
 * @return The length in bytes, or 0 when the file could not be opened.
 * @ghidraAddress 0x00555790
 */
int GetStoredFileLength(const char *pszPath);

/**
 * Report the uncompressed length of a file.
 *
 * It opens the path, measures it from the ark directory record, the gzip trailer, or the file size,
 * and reports the length without reporting the handle, which is why a caller that needs the file
 * opens it again.
 *
 * @param pszPath The file to measure.
 * @return The uncompressed length, or zero or less when the file could not be opened.
 * @ghidraAddress 0x00555800
 */
int GetUncompressedFileLength(const char *pszPath);

/** The window length the decompressor fills before each flush. */
constexpr unsigned kGzipWindowSize = 0x8000;

/**
 * The decompressor window.
 *
 * The image reserves two window lengths, and only the first is written.
 *
 * @ghidraAddress 0x00731468
 */
extern unsigned char g_bGzipWindow[2 * kGzipWindowSize];

/**
 * The write position in g_bGzipWindow.
 *
 * @ghidraAddress 0x00761470
 */
extern unsigned g_nGzipWindowPosition;

/**
 * The staging buffer refilled from the memory source or the file.
 *
 * @ghidraAddress 0x00728c28
 */
extern unsigned char g_bGzipInputBuffer[];

/**
 * The valid byte count in g_bGzipInputBuffer.
 *
 * @ghidraAddress 0x00761468
 */
extern int g_nGzipInputLength;

/**
 * The read position in g_bGzipInputBuffer.
 *
 * @ghidraAddress 0x0076146c
 */
extern int g_nGzipInputPosition;

/**
 * Refill the staging buffer from the memory source or the file and report its first byte.
 *
 * @param nSilentEof Nonzero to report -1 rather than an error when a file has no more bytes.
 * @return The first byte of the refilled buffer, or -1 at a silent end of file.
 * @ghidraAddress 0x006121a0
 */
int GzipRefillInputBuffer(int nSilentEof);

/**
 * Checksum the filled part of g_bGzipWindow, copy it to the output, and rewind the window.
 *
 * An empty window is not flushed.
 *
 * @ghidraAddress 0x006122f8
 */
void GzipFlushWindow();

/**
 * Read one byte through the staging buffer, refilling the buffer when it is exhausted.
 *
 * @return The byte.
 */
inline int GzipGetByte() {
    if (g_nGzipInputPosition < g_nGzipInputLength) {
        return g_bGzipInputBuffer[g_nGzipInputPosition++];
    }
    return GzipRefillInputBuffer(0);
}
