#include "app/rendererbase.h"

void RendererBase::Router::Dispatch(Message *pMsg) {
    mTarget->DispatchPriv(pMsg);
}

void RendererBase::Router::DispatchPriv(Message *) {
}

RendererBase::RendererBase() {
    mRouter.mTarget = this;
    mQueue.AddSink(&mRouter);
}

RendererBase::~RendererBase() {
}

void RendererBase::Dispatch(Message *pMsg) {
    mQueue.Dispatch(pMsg);
}

void RendererBase::Start() {
}

void RendererBase::Stop() {
}

void RendererBase::PollMessages() {
    mQueue.Poll();
}

void RendererBase::UpdateSimple() {
}

void RendererBase::DrawSimple() {
}
