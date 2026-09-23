#include "app/rendererbase.h"

// 0x00139f50
void RendererBase::Router::Handle(Message *pMsg) {
    mTarget->HandleMessage(pMsg);
}

// 0x00139f48
void RendererBase::Router::HandleMessage(Message *) {
}

// 0x00139c10
RendererBase::RendererBase() {
    mRouter.mTarget = this;
    mQueue.AddSink(&mRouter);
}

// 0x00139e20
RendererBase::~RendererBase() {
}

// 0x00139f80
void RendererBase::Handle(Message *pMsg) {
    mQueue.Handle(pMsg);
}

// 0x00139f28
void RendererBase::OnUnknownSlot4() {
}

// 0x00139f30
void RendererBase::OnUnknownSlot5() {
}

// 0x00139fb0
void RendererBase::OnUnknownSlot6() {
    mQueue.Poll();
}

// 0x00139f38
void RendererBase::OnUnknownSlot9() {
}

// 0x00139f40
void RendererBase::OnUnknownSlot10() {
}
