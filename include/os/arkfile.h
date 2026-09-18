#pragma once

/**
 * Mounted ark archive.
 *
 * An ark stores the game's data files in one seekable blob. The streaming layer can therefore read
 * a file without a directory lookup per file. This class is not polymorphic and has no RTTI. Its
 * name comes from the `ArkFile.cpp` allocation tag. Only the operations invoked during start-up
 * have been recovered.
 */
class ArkFile {
public:
    /**
     * Mount an ark archive by path.
     *
     * @param pszPath The archive path, relative to the data root.
     * @return Non-zero on success.
     * @ghidraAddress 0x00559858
     */
    static int Open(const char *pszPath);

    /**
     * Unmount a previously mounted archive.
     *
     * @param pszPath The same path that was passed to Open().
     * @ghidraAddress 0x00559f70
     */
    static void Close(const char *pszPath);
};

/**
 * Mount the archives the game needs for the whole session.
 *
 * Performs no work when ark archives are not in use. On failure the offending path is reported
 * through the failure sink.
 *
 * @return Non-zero when every archive was mounted.
 * @ghidraAddress 0x004dfb20
 */
int InitArk();
