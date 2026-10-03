#pragma once

namespace Rnd {

/**
 * Header of every chunk in a movie file, followed by mSize payload bytes.
 *
 * Its name comes from the debugging symbols of the North American demo release.
 */
struct AMovieChunkHdr {
    unsigned int mTag; /*!< Chunk type, a four-character code such as FRAM. */
    int mTrackId;      /*!< Track the chunk belongs to, an index into the handler slots. */
    int mSize;         /*!< Payload bytes after the header. */
    int mTicks;        /*!< Tick at which the chunk is due, relative to the last loop. */
};

} // namespace Rnd
