#pragma once

#include "os/hxstr.h"

class FailSink;
namespace Rnd {
class Stream;
} // namespace Rnd

namespace Rnd {

/**
 * File name that the renderer resolves against a shared root directory.
 *
 * The class emits no RTTI, and its name is inferred from the later Milo engine, whose `FilePath`
 * plays the same part. Its routines sit in the Rnd::Tex translation unit and take the address of a
 * string member rather than of a texture: Rnd::Tex's bitmap path and Rnd::Movie::mFilename are the
 * two strings they operate on. A path is stored normalised, in lowercase with forward slashes and
 * with `.` and `..` components resolved, and it is written to a file relative to sRoot so that a
 * saved record does not depend on where the data was installed.
 *
 * The class adds no data member to HxStr, so a path is laid out exactly as the string it wraps.
 */
class FilePath : public HxStr {
public:
    /** Construct an empty path. */
    FilePath() {
    }

    /**
     * Construct a path from text without normalising it.
     *
     * @param pszText The text.
     */
    explicit FilePath(const char *pszText) : HxStr(pszText) {
    }

    /**
     * Set the path to name under sRoot.
     *
     * An empty name clears the path. Otherwise the result is sRoot, a slash, and name, normalised.
     * The name is inferred.
     *
     * @param name The file name, relative to sRoot.
     * @ghidraAddress 0x004e4bd8
     */
    void SetFromRoot(const HxStr &name);

    /**
     * Set the path to path verbatim, then normalise it. The name is inferred.
     *
     * @param path The full path.
     * @ghidraAddress 0x004e7b70
     */
    void Set(const HxStr &path);

    /**
     * Lowercase the path, turn backslashes into slashes, and resolve its components.
     *
     * The path is split on slashes. A `..` component removes the component before it, and every
     * other component that begins with a full stop is dropped, `.` included. The shipped build
     * treats a name such as `.hidden` the same way. The remaining components are joined with single
     * slashes, so a leading slash is not preserved. The name is inferred.
     *
     * @ghidraAddress 0x004e4d88
     */
    void Normalize();

    /**
     * Express the path relative to sRoot.
     *
     * The leading components the path shares with sRoot are dropped. Each remaining component of
     * sRoot contributes a `..`, and the remaining components of the path follow. The result lives
     * in a function-local static string, so it is valid only until the next call. An empty path is
     * returned unchanged. The name is inferred.
     *
     * @return The relative path.
     * @ghidraAddress 0x004e5168
     */
    const HxStr &RelativeToRoot() const;

    /**
     * Write the path to sink in double quotes.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x004e7bc0
     */
    void Print(FailSink &sink) const;

    /**
     * Write the path relative to sRoot, with its terminator, to stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004e7bf8
     */
    void Save(Stream &stream) const;

    /**
     * Read a name from stream and set the path to it under sRoot.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x004e7c50
     */
    void Load(Stream &stream);

    /**
     * Replace sRoot and normalise it.
     *
     * Rnd::AsyncLoader::PollAsyncLoads(), ScriptSaveRnd(), and a routine at `0x004162cc` call it.
     * The name is inferred.
     *
     * @param root The new root directory.
     * @ghidraAddress 0x004e78d8
     */
    static void SetRoot(const HxStr &root);

    /**
     * Directory every relative path is resolved against, empty until SetRoot() is called.
     *
     * The Rnd::Tex unit's static initialiser constructs it after Rnd::g_texClassName.
     *
     * @ghidraAddress 0x007033b8
     */
    static FilePath sRoot;
};

} // namespace Rnd
