#pragma once

#include <list>

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/filepath.h"
#include "rnd/manager.h"
#include "rnd/moviestream.h"
#include "rndartt/apalette.h"

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
 * A movie plays its file through one Rnd::MovieStream, which it owns and deletes in
 * CloseMovieFile(). Each track of the file is attached to the texture mTrackTextures names for
 * it, and the stream passes that track's video chunks to OnChunk() as SetFrameSelf() advances it.
 */
class Movie : public Animatable {
public:
    /**
     * One video track of the file and the texture it plays into.
     *
     * The title is inferred from the dump at `0x005d15e0`, which prints the list under the label
     * "trackTextures:" and each entry as "(trackId:" and " tex:". The record is 8 bytes, which the
     * list node the reader builds pins: the node is 0x10 bytes with the payload at `+0x08`.
     */
    struct TrackTexture {
        int mTrackId; /*!< Track of the movie file this entry plays. +0x00 */
        Tex *mTex;    /*!< Texture the track plays into, resolved by object name. +0x04 */
    };

    /**
     * Construct a movie with no file, no track textures, and a cleared stream buffer.
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
     * Close the file and release every track texture.
     *
     * @ghidraAddress 0x005ce930
     */
    virtual ~Movie();

    /**
     * Read the movie record from stream.
     *
     * The first word is the record revision. A revision of 3 or above reports "Can't load new
     * Movie" and abandons the record. The base record follows, then the file is closed, then
     * mFilename is read through FilePath::Load(), which resolves it against FilePath::sRoot. A
     * revision below 2 then reads and discards one byte. The track texture list follows, and the
     * file is reopened from the name just read.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x005d22f8
     */
    virtual void Load(Stream &stream);

    /**
     * Write a description of this movie to sink.
     *
     * Rnd::Object and then Rnd::Animatable describe themselves first. At a positive dump level a
     * "[Movie]" section follows with mFilename and mTrackTextures, the list through the printer at
     * `0x005d15e0`.
     *
     * @param sink The text sink.
     * @ghidraAddress 0x005d21e0
     */
    virtual void DumpText(FailSink &sink);

    /**
     * Serialise the movie.
     *
     * Writes revision 2, then the Rnd::Animatable record, mFilename relative to FilePath::sRoot,
     * and mTrackTextures through the writer at `0x005d1790`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x005d2280
     */
    virtual void Save(Stream &stream);

    /**
     * Replace one object reference with another.
     *
     * Forwards to Rnd::Animatable, then retargets every track texture that is pFrom, moving this
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
     * copies mFilename and mTrackTextures, and opens the file again. The cast result is used
     * without a null check, so a source that is not a movie faults.
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
     * Return the texture of the first entry of mTrackTextures that plays nTrackId.
     *
     * The routine has no caller in the shipped build, and its name is inferred.
     *
     * @param nTrackId The track to look up.
     * @return The track's texture, or null when no entry plays it.
     * @ghidraAddress 0x005d2480
     */
    Tex *FindTrackTexture(int nTrackId) const;

    /**
     * Return mFilename relative to FilePath::sRoot.
     *
     * The routine has no caller in the shipped build, and its name is inferred.
     *
     * @return The relative path, valid until the next relative path is taken.
     * @ghidraAddress 0x005d2058
     */
    const HxStr &GetRelativeFilename() const;

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
     * Set the file this movie plays, under FilePath::sRoot.
     *
     * Hands the name to FilePath::SetFromRoot(), which FilePath::Load() also ends in, so a name set
     * this way and a name read from a file are resolved against the same directory. Neither opens
     * the file; Reopen() does that.
     *
     * @param name The file name, without the `.mmv` extension the open path appends.
     * @ghidraAddress 0x005d2190
     */
    void SetFilename(const HxStr &name);

    /**
     * Open the file mFilename names and take a reference on every track texture.
     *
     * Adds this object as a referrer on each track texture first, then returns at once when
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
     * Release every track texture and destroy the stream reader.
     *
     * @ghidraAddress 0x005cef00
     */
    void CloseMovieFile();

    /**
     * Route one track's chunks to this movie.
     *
     * Installs ChunkHandler() with this movie as its data. Does nothing when no file is open.
     *
     * @param nTrackId The track.
     * @ghidraAddress 0x005d24d8
     */
    void AttachTrack(int nTrackId);

    /**
     * Stop routing one track's chunks anywhere. Does nothing when no file is open.
     *
     * @param nTrackId The track.
     * @ghidraAddress 0x005d2508
     */
    void DetachTrack(int nTrackId);

    /**
     * Play a track into a texture, replacing any texture the track played into before.
     *
     * The texture takes a reference from this movie and is appended to mTrackTextures. With a file
     * open, a texture whose size or depth differs from the track's frame is logged and reconfigured
     * as an 8 bit bitmap of the frame's size. The track is then attached. A null texture only
     * removes the track. The routine has no caller in the shipped build, and its name is inferred.
     *
     * @param nTrackId The track.
     * @param pTex The texture, or null.
     * @ghidraAddress 0x005cf1f8
     */
    void SetTrackTexture(int nTrackId, Tex *pTex);

    /**
     * Drop the first entry of mTrackTextures that plays nTrackId, releasing its texture, and detach
     * the track. The name is inferred.
     *
     * @param nTrackId The track.
     * @ghidraAddress 0x005cf3f8
     */
    void RemoveTrackTexture(int nTrackId);

    /**
     * Draw one video chunk into the texture its track plays into.
     *
     * A palette chunk is copied into mPalette and handed to the texture. A frame chunk's bitmap
     * takes mPalette and is copied onto the locked mip 0 at the chunk's position. A blank chunk
     * fills mip 0 with its colour. A track with no texture is ignored. The name is inferred.
     *
     * @param pHeader The chunk.
     * @param pPayload The chunk's payload.
     * @ghidraAddress 0x005cf4c0
     */
    void OnChunk(MovieStream::ChunkHeader *pHeader, void *pPayload);

    /**
     * The MovieStream handler AttachTrack() installs, which passes the chunk to pData's OnChunk().
     *
     * The routine was an orphan in the analysis, and its name is inferred.
     *
     * @param pHeader The chunk.
     * @param pPayload The chunk's payload.
     * @param pData The movie.
     * @ghidraAddress 0x005d2530
     */
    static void ChunkHandler(MovieStream::ChunkHeader *pHeader, void *pPayload, void *pData);

    /**
     * Read a track texture list from stream.
     *
     * The first word is the entry count, and the list is resized to it before the entries are
     * read. Each entry is a track id and the object name of its texture, and the name is resolved
     * through Rnd::g_manager and narrowed to Rnd::Tex. A name that resolves to nothing, or to an
     * object that is not a texture, stores a null texture rather than failing the record.
     *
     * The routine takes no movie. It is placed here because Movie::TrackTexture is the only type it
     * builds, and Load() is its only caller.
     *
     * @param stream The stream to read from.
     * @param textures Receives the entries.
     * @ghidraAddress 0x005d1a90
     */
    static void ReadTrackTextures(Stream &stream, std::list<TrackTexture> &textures);

    /**
     * File this movie plays, resolved against FilePath::sRoot. Public because FilePath's routines
     * take its address from outside the hierarchy and the image exposes no accessor. +0x18
     */
    FilePath mFilename;

private:
    MovieStream *mStream; // +0x20 The open file, or null.
    // +0x24 Set by a successful open. The next SetFrameSelf() starts the stream's loop ticks at
    // the current tick and clears it.
    int mStartPending;
    // +0x28 Set once SetFrameSelf() has matched every track texture to its frame, and cleared by a
    // successful open.
    int mTexturesMatched;
    int mZone; // +0x2c The zone current when the file was opened, -1 before.

public:
    /**
     * Textures the file's tracks play into. Public because ReadTrackTextures() receives its
     * address from Load() and fills it from outside the class, and the image exposes no accessor.
     * +0x30
     */
    std::list<TrackTexture> mTrackTextures;

private:
    // +0x34 Palette the PALL chunks fill and every frame chunk's bitmap draws through. The
    // constructor also clears it with memset() after constructing it.
    APalette mPalette;
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
 * converting the result to its Rnd::Object subobject. The name is inferred. An exception from the
 * constructor produces null, which is what the binary's handler returns.
 *
 * @param name The object name.
 * @return The new movie, or null.
 * @ghidraAddress 0x005d1f28
 */
inline Movie *NewMovie(const HxStr &name) {
    try {
        return new Movie(name);
    } catch (...) {
        return nullptr;
    }
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
