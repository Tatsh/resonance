#include "app/rendererbase.h"

// NTSC-U/C: 0x00139f50, PAL: 0x0013a898
void RendererBase::Router::Dispatch(Message *pMsg) {
    mTarget->DispatchPriv(pMsg);
}

// NTSC-U/C: 0x00139f48, PAL: 0x0013a890
void RendererBase::Router::DispatchPriv(Message *) {
}

// NTSC-U/C: 0x00139c10, PAL: 0x0013a558
RendererBase::RendererBase() {
    mRouter.mTarget = this;
    mQueue.AddSink(&mRouter);
}

// NTSC-U/C: 0x00139e20, PAL: 0x0013a768
RendererBase::~RendererBase() {
}

// NTSC-U/C: 0x00139f80, PAL: 0x0013a8c8
void RendererBase::Dispatch(Message *pMsg) {
    mQueue.Dispatch(pMsg);
}

// NTSC-U/C: 0x00139f28, PAL: 0x0013a870
void RendererBase::Start() {
}

// NTSC-U/C: 0x00139f30, PAL: 0x0013a878
void RendererBase::Stop() {
}

// NTSC-U/C: 0x00139fb0, PAL: 0x0013a8f8
void RendererBase::PollMessages() {
    mQueue.Poll();
}

// NTSC-U/C: 0x00139f38, PAL: 0x0013a880
void RendererBase::UpdateSimple() {
}

// NTSC-U/C: 0x00139f40, PAL: 0x0013a888
void RendererBase::DrawSimple() {
}
