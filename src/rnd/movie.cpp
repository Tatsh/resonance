#include "rnd/movie.h"

#include <list>

#include "os/hxstr.h"
#include "os/mem.h"
#include "rnd/manager.h"
#include "rnd/stream.h"
#include "rnd/tex.h"
#include "rndartt/apalette.h"

namespace Rnd {

namespace {

const char *const kMovieTag = "Rnd::Movie";

} // namespace

// 0x005d1a90
void Movie::ReadClipList(Stream &stream, std::list<Clip> &clips) {
    int nCount;
    stream.Read(&nCount, sizeof(nCount));
    clips.resize(nCount);

    for (auto &clip : clips) {
        stream.Read(&clip.mFrames, sizeof(clip.mFrames));

        HxStr name;
        stream.ReadString(name);
        clip.mTarget = dynamic_cast<Tex *>(g_manager.Find(name));
    }
}

// 0x005d21b0
void Movie::Reopen() {
    CloseMovieFile();
    OpenMovieFile();
}

// 0x005d2078
const HxStr &Movie::ClassName() const {
    return g_movieClassName;
}

// 0x005d23c8
void Movie::Copy(const Object *pSource, unsigned nFlags) {
    const Movie *pMovie = dynamic_cast<const Movie *>(pSource);
    Animatable::Copy(pSource, nFlags);
    CloseMovieFile();
    mFilename = pMovie->mFilename;
    mClips = pMovie->mClips;
    OpenMovieFile();
}

// 0x005d1eb8
void *Movie::operator new(size_t nSize) {
    return AllocateTaggedMemory(nSize, kMovieTag);
}

// 0x005d1ed8
void Movie::operator delete(void *pBlock) {
    FreeTaggedMemory(pBlock, kMovieTag);
}

// 0x005d2480
Tex *Movie::FindClipTarget(int nFrames) const {
    for (const auto &clip : mClips) {
        if (clip.mFrames == nFrames) {
            return clip.mTarget;
        }
    }
    return nullptr;
}

// 0x005d20b8
Object *CreateRegisteredMovie(const HxStr &name) {
    return NewMovie(name);
}

// 0x0077a590
HxStr g_movieClassName("Movie");

} // namespace Rnd
