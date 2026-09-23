#pragma once

#include <list>

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"

class FailSink;
namespace Rnd {
class Stream;
class Tex;
} // namespace Rnd

namespace Rnd {

/**
 * Animation that streams a compressed video file into a set of textures.
 *
 * `Q23Rnd5Movie` in the RTTI descriptor at `0x008efc90`, with Rnd::Animatable as its only public
 * non-virtual base at offset 0. Rnd::Manager::RemapLegacyClassName() rewrites the earlier title
 * `TexMovie` to `Movie` for any file below format version 5, which is what identifies the class as
 * the texture-streaming one rather than a video surface of its own.
 *
 * The object is 0x458 bytes, which the factory at `0x005d20b8` pins by requesting exactly that
 * many under the tag "Rnd::Movie" at `0x00835ba8`. Rnd::Animatable derives from Rnd::Object
 * virtually, so the Rnd::Object subobject is placed last, at `+0x43c`, which the `-0x43c`
 * adjustment on every entry of the Rnd::Object vtable below confirms. That subobject is 0x18
 * bytes, so four bytes of tail padding follow it.
 *
 * Two vtables belong to the class. The eight-entry table at `0x00835d60` is addressed by the
 * Rnd::Object subobject vptr and overrides seven of the eight Rnd::Object slots. The five-entry
 * table at `0x00835da8` is addressed by the Animatable vptr at `+0x14`; it inherits EndFrame() and
 * StartAnim() from Rnd::Animatable, overrides SetFrameSelf(), and adds the one virtual this class
 * declares. Both tables end in an all-zero entry, which is a terminator rather than a slot. Reading
 * the second table's terminator as a slot would have run it into the literal pool that follows it,
 * where the bytes decode as further plausible-looking entries.
 *
 * A movie owns one stream reader, a separately allocated object of 0x380 bytes whose constructor is
 * at `0x0057f7b8`, whose destructor is at `0x00580858`, whose per-frame pump is at `0x0057fac8`,
 * and whose advance hook is at `0x005808d0`. The class of that object has no recoverable title.
 * It emits no RTTI descriptor, it is allocated through the untagged MemAllocScalar() rather than
 * under a tag, its own error text is generic ("Out of memory", "Can't open file", "Read error"),
 * and the image holds no embedded source path for it. Its member is therefore absent from the
 * declarations below rather than declared under an invented type. The two companion descriptors
 * `MovieAsyncCallback` at `0x009022c0` and `MovieStreamingAsyncCallback` at `0x008ef680` belong to
 * the same cluster and are the likeliest route to a title.
 *
 * Four further members sit between mFilename and mClips and are recovered by behaviour only. The
 * stream reader occupies `+0x20`, owned outright and deleted by CloseMovieFile(). `+0x24` becomes 1
 * when the file opened without error. `+0x28` is cleared on a successful open. `+0x2c` receives
 * ZoneGetCurrent() at the moment of the open and starts at -1. A 0x408-byte region follows mClips,
 * at `+0x34` through `+0x43b`, which the constructor clears with one memset() and never otherwise
 * touches. The unreferenced literal "movieStreamBuff" at `0x0081c9b0` is a candidate title for it
 * and nothing ties the two together, so the region stays undeclared as well.
 */
class Movie : public Animatable {
public:
    /**
     * One video segment, as a frame count and the texture it plays into.
     *
     * The title is inferred. The record is 8 bytes, which the list node the reader builds pins: the
     * node is 0x10 bytes with the payload at `+0x08`, and the reader writes a word at `+0x08` and
     * the texture at `+0x0c`.
     */
    struct Clip {
        int mFrames;  /*!< Length of this segment, read straight from the file. +0x00 */
        Tex *mTarget; /*!< Texture this segment plays into, resolved by object name. +0x04 */
    };

    /**
     * Construct a movie with no file, no clips, and a cleared stream buffer.
     *
     * The stream reader starts null, the opened flag clear, and the recorded zone at -1. The
     * routine takes only the object name; the second argument the disassembly shows is the
     * compiler's virtual-base construction flag, which the factory passes as 1.
     *
     * @param name The registry key for this object.
     * @ghidraAddress 0x005ce3d0
     */
    explicit Movie(const HxStr &name);

    /**
     * Close the file and release every clip texture.
     *
     * @ghidraAddress 0x005ce930
     */
    virtual ~Movie();

    /**
     * Read the movie record from stream.
     *
     * The first word is the record revision. A revision of 3 or above reports "Can't load new
     * Movie" and abandons the record. The base record follows, then the file is closed, then
     * mFilename is read through the bitmap-path reader, which is what applies the texture
     * directory to it. A revision below 2 then reads and discards one byte. The clip list follows,
     * and the file is reopened from the name just read.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x005d22f8
     */
    virtual void Load(Stream &stream);

    /**
     * Write a description of this movie to sink.
     *
     * The Rnd::Animatable description is followed by mFilename through the file path printer at
     * `0x004e7bc0` and the clip list through the printer at `0x005d15e0`.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x005d21e0
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the movie.
     *
     * The Rnd::Animatable record is followed by mFilename through the file path writer at
     * `0x004e7bf8` and the clip list through the writer at `0x005d1790`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x005d2280
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * Forwards to Rnd::Animatable, then retargets every clip whose target is pFrom, moving this
     * object's reference, and finally notifies the stream reader.
     *
     * @param pFrom The object being replaced.
     * @param pTo The object to point at, which may be null.
     * @ghidraAddress 0x005ced80
     */
    virtual void Replace(Object *pFrom, Object *pTo);

    /**
     * Report the class key a `.rnd` file writes for a movie.
     *
     * @return g_movieClassName.
     * @ghidraAddress 0x005d2078
     */
    virtual const HxStr &ClassName() const;

    /**
     * Copy another movie over this one.
     *
     * Narrows the source with a dynamic cast, runs the Rnd::Animatable copy, closes the file,
     * copies mFilename and mClips, and opens the file again. The cast result is used without a
     * null check, so a source that is not a movie faults.
     *
     * @param pSource The source object.
     * @param nFlags The copy flags, passed to the base.
     * @ghidraAddress 0x005d23c8
     */
    virtual void Copy(const Object *pSource, unsigned nFlags);

    /**
     * Allocate a movie under the tag "Rnd::Movie".
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x005d1eb8
     */
    static void *operator new(size_t nSize);

    /**
     * Release a movie block under the same tag.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x005d1ed8
     */
    static void operator delete(void *pBlock);

    /**
     * Return the target of the first clip whose frame count equals nFrames.
     *
     * The routine has no caller in the shipped build, and its name is inferred.
     *
     * @param nFrames The frame count to match.
     * @return The clip's texture, or null when no clip matches.
     * @ghidraAddress 0x005d2480
     */
    Tex *FindClipTarget(int nFrames) const;

    /**
     * Advance the movie to a frame.
     *
     * Animatable vtable slot 3.
     *
     * @param flFrame The frame to animate to, after this object's filter chain.
     * @ghidraAddress 0x005cef88
     */
    virtual void SetFrameSelf(float flFrame);

    /**
     * Reopen the file this movie already names.
     *
     * Animatable vtable slot 4, and the one virtual this class adds. The body is CloseMovieFile()
     * followed by OpenMovieFile() and it takes no argument, so the routine reloads rather than
     * assigning a name. Nothing in the image calls it or overrides it.
     *
     * @ghidraAddress 0x005d21b0
     */
    virtual void Reopen();

    /**
     * Set the file this movie plays, under the texture directory.
     *
     * Hands the name to the bitmap-path setter at `0x004e4bd8`, which is the same routine Load()
     * reads mFilename through, so a name set this way and a name read from a file are resolved
     * against the same directory. Neither opens the file; Reopen() does that.
     *
     * @param name The file name, without the `.mmv` extension the open path appends.
     * @ghidraAddress 0x005d2190
     */
    void SetFilename(const HxStr &name);

    /**
     * Open the file mFilename names and take a reference on every clip texture.
     *
     * Adds this object as a referrer on each clip target first, then returns at once when
     * mFilename is empty. The path is mFilename with ".mmv" appended. A path whose uncompressed
     * length reads as zero reports "Couldn't load movie file: %s" and returns. Otherwise the
     * current zone is recorded, a 0x380-byte stream reader is allocated, and its constructor is
     * run against the path. A constructor that reports an error writes the text from the table at
     * `0x007a8400`, indexed by the negative error code, into "Couldn't load movie file %s: %s",
     * then destroys the reader and clears the member.
     *
     * @ghidraAddress 0x005cebe0
     */
    void OpenMovieFile();

    /**
     * Release every clip texture and destroy the stream reader.
     *
     * @ghidraAddress 0x005cef00
     */
    void CloseMovieFile();

    /**
     * Drive the stream reader forward by one step.
     *
     * Does nothing when no file is open. The reader's advance hook at `0x005808d0` receives the
     * callback at `0x005d2530`, which Ghidra has not claimed as a routine.
     *
     * @ghidraAddress 0x005d24d8
     */
    void UpdateSubObject(Stream &stream);

    /**
     * Read a clip list from stream.
     *
     * The first word is the clip count, and the list is resized to it before the entries are read.
     * Each entry is a frame count and the object name of its target, and the name is resolved
     * through Rnd::g_manager and narrowed to Rnd::Tex. A name that resolves to nothing, or to an
     * object that is not a texture, stores a null target rather than failing the record.
     *
     * The routine takes no movie. It is placed here because Movie::Clip is the only type it
     * builds, and Load() is its only caller.
     *
     * @param stream The stream to read from.
     * @param clips Receives the clips.
     * @ghidraAddress 0x005d1a90
     */
    static void ReadClipList(Stream &stream, std::list<Clip> &clips);

    HxStr mFilename; /*!< File this movie plays, resolved against the texture directory. Public
                          because the bitmap-path helpers at `0x004e4bd8` and `0x004e7c50` take its
                          address from outside the hierarchy and the image exposes no accessor.
                          +0x18 */

    /** Video segments in play order. Public because the reader at `0x005d1a90` receives its address
        from Load() and fills it from outside the class, and the image exposes no accessor. +0x30 */
    std::list<Clip> mClips;
};

/**
 * Allocate and construct a movie.
 *
 * The binary bills the allocation to the tag "Rnd::Movie" and requests exactly 0x458 bytes.
 * Rnd::Manager::Init() registers this factory for the type name "Movie".
 *
 * @param name The object name.
 * @return The new movie, as its Rnd::Object subobject.
 * @ghidraAddress 0x005d20b8
 */
Object *CreateRegisteredMovie(const HxStr &name);

/**
 * Allocate and construct a movie, returning the Movie pointer itself.
 *
 * The out-of-line copy has no caller, and CreateRegisteredMovie() expands the body before
 * converting the result to its Rnd::Object subobject. The name is inferred.
 *
 * @param name The object name.
 * @return The new movie.
 * @ghidraAddress 0x005d1f28
 */
inline Movie *NewMovie(const HxStr &name) {
    return new Movie(name);
}

/**
 * Registered class name of Rnd::Movie, the string "Movie".
 *
 * @ghidraAddress 0x0077a590
 */
extern HxStr g_movieClassName;

/**
 * Register the "Movie" class with Rnd::Manager.
 *
 * The class has no creator hook, so the body is the registration alone. The out-of-line copy has
 * no caller, and Rnd::Manager::Init() registers the class itself. The name is inferred.
 *
 * @ghidraAddress 0x005d1ef8
 */
inline void RegisterMovieClass() {
    g_manager.RegisterClass(g_movieClassName, CreateRegisteredMovie);
}

} // namespace Rnd
