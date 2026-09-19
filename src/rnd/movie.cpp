#include "rnd/movie.h"

#include <list>

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/stream.h"
#include "rnd/tex.h"

namespace Rnd {

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

} // namespace Rnd
