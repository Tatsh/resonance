#include "rnd/asyncloader.h"

#include "rnd/manager.h"
#include "rnd/tex.h"
#include "rnd/text.h"

// 0x003f7e50
RndAsyncLoader::RndAsyncLoader() : mPending(1), mStarted(0), mFinished(0), mPriority(-1) {
}

// 0x003f8460
void RndAsyncLoader::HarvestLoadedObjects() {
    mUnknown08 = Rnd::g_manager.mLoaded;

    for (auto it = Rnd::g_manager.mLoaded.begin(); it != Rnd::g_manager.mLoaded.end(); ++it) {
        if ((*it)->ClassName() == "Tex") {
            mObjects.push_back(dynamic_cast<Rnd::Tex *>(*it)); // The binary's cast helper.
        }
        // The binary calls ClassName() again rather than testing with an else.
        if ((*it)->ClassName() == "Text") {
            mDrawables.push_back(dynamic_cast<Rnd::Text *>(*it)); // The binary's cast helper.
        }
    }
    for (auto it = Rnd::g_manager.mMergeObjects.begin(); it != Rnd::g_manager.mMergeObjects.end();
         ++it) {
        if ((*it)->ClassName() == "Tex") {
            mObjects.push_back(dynamic_cast<Rnd::Tex *>(*it)); // The binary's cast helper.
        }
        if ((*it)->ClassName() == "Text") {
            mDrawables.push_back(dynamic_cast<Rnd::Text *>(*it)); // The binary's cast helper.
        }
    }
}

// 0x003fc708
void RndAsyncLoader::Restart(const HxStr &directory, const HxStr &file) {
    Cancel();
    mDirectory = directory;
    mFile = file;
    mFinished = 0;
    mPending = 1;
    mStarted = 0;
}
